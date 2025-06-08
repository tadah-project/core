/**
 *  @file blueprint_validator.cpp
 *  @brief Implementation of tadah::core::BlueprintValidator
 *
 *  – validate() now detects lines that are “just a long list of
 *    numbers” more reliably and skips std::regex for them.
 */

#include <tadah/core/blueprint_validator.h>

#include <regex>
#include <unordered_map>
#include <algorithm>
#include <sstream>
#include <cerrno>
#include <cstdlib>
#include <cctype>
#include <toml.hpp>

namespace tadah {
namespace core {

// ──────────────────────────────────────────────────────────────
// Primitive fragments
// ──────────────────────────────────────────────────────────────
static const std::unordered_map<std::string,std::string> g_primitiveMap = {
  {"INT",    "\\s*[-+]?\\d+\\s*"},
  {"DOUBLE", "\\s*[-+]?\\d+(?:\\.\\d+)?(?:[eE][-+]?\\d+)?\\s*"},
  {"STRING", "\\s*[^\\s]*[A-Za-z*][^\\s]*\\s*"},
  {"UINT",   "\\s*(?:0|[1-9]\\d*)\\s*"},
  {"BOOL",   "\\s*(?:true|false)\\s*"},
  {"INDEX_PATTERN",
   "\\s*[+]?\\d+(?:\\s*-\\s*[-+]?\\d+(?:\\s*:\\s*[+]?\\d+)?){0,1}\\s*" }
};

// ──────────────────────────────────────────────────────────────
// Small utilities
// ──────────────────────────────────────────────────────────────
static std::string trim(const std::string& s)
{
  const auto b = s.find_first_not_of(" \t\r\n");
  if (b==std::string::npos) return "";
  const auto e = s.find_last_not_of(" \t\r\n");
  return s.substr(b,e-b+1);
}

static std::vector<std::string> splitBlueprint(const std::string& bp)
{
  std::vector<std::string> out;
  std::string tok;
  bool inGroup=false; char end=0;

  for(char c:bp){
    if(!inGroup&&(c=='['||c=='{')){
      if(!tok.empty()){out.push_back(trim(tok));tok.clear();}
      inGroup=true; end=(c=='[')?']':'}'; tok.push_back(c);
    }
    else if(inGroup){
      tok.push_back(c);
      if(c==end){ inGroup=false; out.push_back(trim(tok)); tok.clear(); }
    }
    else{
      if(std::isspace((unsigned char)c)){ if(!tok.empty()){out.push_back(trim(tok));tok.clear();} }
      else tok.push_back(c);
    }
  }
  if(!tok.empty()) out.push_back(trim(tok));
  return out;
}

static std::vector<std::string>
reorderAlternatives(std::vector<std::string> alts)
{
  auto rank=[](const std::string&s){ if(s=="UINT")return 1; if(s=="INT")return 2;
                                     if(s=="DOUBLE")return 3; return 9999; };
  std::sort(alts.begin(),alts.end(),
            [&](const std::string&a,const std::string&b){return rank(a)<rank(b);});
  return alts;
}

// ──────────────────────────────────────────────────────────────
// Forward declarations
// ──────────────────────────────────────────────────────────────
static std::string buildTokenRegex(const std::string&);
static std::string buildRegexFromBlueprint(const std::string&,bool);

// ──────────────────────────────────────────────────────────────
// STACK-SAFE detector for “long numeric line”
// ──────────────────────────────────────────────────────────────
static bool looksLikeLongNumericLine(const std::string& text)
{
  if (text.size() < 2048)             // only bother for long lines
      return false;

  const char* p = text.c_str();
  char* end    = nullptr;
  bool sawAtLeastOne = false;

  while (true)
  {
    // skip any amount of white-space or commas
    while (*p && (std::isspace((unsigned char)*p) || *p==',')) ++p;
    if (*p == '\0') break;

    errno = 0;
    std::strtod(p, &end);
    if (end == p || errno == ERANGE)  // not a valid double
        return false;

    sawAtLeastOne = true;
    p = end;
  }
  return sawAtLeastOne;
}

// ──────────────────────────────────────────────────────────────
// Token-level regex helpers
// ──────────────────────────────────────────────────────────────
static std::string buildTokenRegex(const std::string& tok)
{
  if (tok.empty()) return "";

  if (tok.front()=='{' && tok.back()=='}') {
    const std::string inner = trim(tok.substr(1,tok.size()-2));
    const auto subs = splitBlueprint(inner);

    std::string block; bool first=true;
    for(const auto& s:subs){
      if(!first) block += "[,\\s+]";
      block += buildTokenRegex(s);
      first=false;
    }
    return "(?:"+block+")(?:[,\\s+]"+block+")*";
  }

  if (tok.front()=='[' && tok.back()==']') {
    std::string inner = trim(tok.substr(1,tok.size()-2));
    for(std::size_t pos=0;(pos=inner.find(" or ",pos))!=std::string::npos;)
      { inner.replace(pos,4,"|"); ++pos; }

    std::vector<std::string> alts;
    std::size_t start=0;
    while(true){
      auto bar=inner.find('|',start);
      if(bar==std::string::npos){ alts.push_back(trim(inner.substr(start))); break; }
      alts.push_back(trim(inner.substr(start,bar-start)));
      start=bar+1;
    }
    if(alts.size()>1) alts=reorderAlternatives(alts);

    std::string re="(?:";
    for(std::size_t i=0;i<alts.size();++i){
      auto it=g_primitiveMap.find(alts[i]);
      re+=(it!=g_primitiveMap.end())?it->second:alts[i];
      if(i+1<alts.size()) re+="|";
    }
    re+=")";
    return re;
  }

  auto it=g_primitiveMap.find(tok);
  return (it!=g_primitiveMap.end()) ? it->second : tok;
}

// blueprint-level
static std::string buildRegexFromBlueprint(const std::string& bp,bool cap)
{
  const auto toks = splitBlueprint(bp);
  if(toks.empty()) return "^(?!).*$";

  auto open=[&](bool want){return std::string(want?"(":"(?:");};
  const std::string close=")";

  std::string pat="^";
  bool first=true;
  for(const auto& t:toks){
    bool opt=(!t.empty()&&t.front()=='['&&t.back()==']');
    if(first){
      first=false;
      pat+= opt ? open(cap)+buildTokenRegex(t)+")?"
                : open(cap)+buildTokenRegex(t)+close;
    }else{
      if(opt)
        pat+="(?:\\s+"+open(cap)+buildTokenRegex(t)+close+")?";
      else
        pat+="\\s+"  +open(cap)+buildTokenRegex(t)+close;
    }
  }
  pat+="\\s*$";
  return pat;
}
static std::string buildRegexFromBlueprint(const std::string& bp)
{ return buildRegexFromBlueprint(bp,true); }

// ──────────────────────────────────────────────────────────────
// pImpl
// ──────────────────────────────────────────────────────────────
class BlueprintValidator::Impl {
public:
  explicit Impl(const std::vector<std::string>& pats){
    validateRegex.reserve(pats.size());
    for(auto& p:pats)
      validateRegex.emplace_back(p,
        std::regex::ECMAScript|std::regex::optimize|std::regex::nosubs);
  }
  ~Impl(){
    for(auto&r:validateRegex) std::regex{}.swap(r);
    std::vector<std::regex>().swap(validateRegex);
  }
  std::vector<std::regex> validateRegex;
};

// ──────────────────────────────────────────────────────────────
// Constructors
// ──────────────────────────────────────────────────────────────
BlueprintValidator::BlueprintValidator(const std::string& bp)
{
  m_patterns = buildRegexFromBlueprint(bp,true);
  std::vector<std::string> nc { buildRegexFromBlueprint(bp,false) };
  m_impl = new Impl(nc);
}

BlueprintValidator::BlueprintValidator(const std::vector<std::string>& bps)
{
  std::vector<std::string> nonCap;
  bool first=true;
  for(auto& bp:bps){
    const auto cap=buildRegexFromBlueprint(bp,true);
    if(!first) m_patterns+='\n';
    m_patterns+=cap; first=false;
    nonCap.push_back(buildRegexFromBlueprint(bp,false));
  }
  m_impl = new Impl(nonCap);
}

// ──────────────────────────────────────────────────────────────
// validate()
// ──────────────────────────────────────────────────────────────
bool BlueprintValidator::validate(const std::string& text) const
{
  if (looksLikeLongNumericLine(text))
      return true;                        // skip regex, avoid recursion

  if(!m_impl) return false;
  for(auto& r:m_impl->validateRegex)
    if(std::regex_match(text,r))
      return true;
  return false;
}

bool BlueprintValidator::validate(const std::vector<std::string>& toks) const
{
  std::string concat;
  for(std::size_t i=0;i<toks.size();++i){
    concat+=toks[i];
    if(i+1<toks.size()) concat+=' ';
  }
  return validate(concat);
}

// ──────────────────────────────────────────────────────────────
// getCaptures() – unchanged logic
// ──────────────────────────────────────────────────────────────
std::vector<std::string>
BlueprintValidator::getCaptures(const std::string& text) const
{
  std::vector<std::string> out;
  if(m_patterns.empty()) return out;

  int best=-1; std::smatch bestMatch;
  std::istringstream iss(m_patterns); std::string line;

  while(std::getline(iss,line)){
    if(line.empty()) continue;
    std::regex cre(line,std::regex::ECMAScript);
    std::smatch m;
    if(std::regex_match(text,m,cre)){
      for(std::size_t i=1;i<m.size();++i) out.push_back(m[i].str());
      return out;
    }else if(std::regex_search(text,m,cre)){
      int len=(int)m.length(0); if(len>best){best=len; bestMatch=m;}
    }
  }
  if(out.empty()&&best>0)
    for(std::size_t i=1;i<bestMatch.size();++i) out.push_back(bestMatch[i].str());
  return out;
}

// convenience helpers / destructor
bool BlueprintValidator::validateOnce(const std::string& bp,const std::string& txt){
  std::regex r(buildRegexFromBlueprint(bp,false),
               std::regex::ECMAScript|std::regex::nosubs);
  return std::regex_match(txt,r);
}
bool BlueprintValidator::validateOnce(const std::vector<std::string>& bps,
                                      const std::string& txt){
  BlueprintValidator v(bps); return v.validate(txt);
}
std::string BlueprintValidator::getPattern() const { return m_patterns; }
BlueprintValidator::~BlueprintValidator(){ delete m_impl; m_impl=nullptr; }

} // namespace core
} // namespace tadah
