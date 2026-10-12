/**
 *  @file blueprint_validator.cpp
 *  @brief Implementation of tadah::core::BlueprintValidator
 *
 *  The values of a list are separated by blanks, as CoreUserConfigParser
 *  splits a configuration line into values at blanks only: a comma or a
 *  '+' between two values is not a separator (a STRING word keeps them;
 *  an index list, INDEX_PATTERN, takes commas between its entries, which
 *  its readers split at). Lines of 2048 characters or more are checked
 *  without std::regex, which overflows the stack on long input, by the
 *  same rules.
 */

#include <tadah/core/blueprint_validator.h>

#include <regex>
#include <unordered_map>
#include <algorithm>
#include <sstream>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cctype>
#include <toml.hpp>

namespace tadah {
namespace core {

enum class LongLineValidationResult {
  kNotApplicable,
  kValid,
  kInvalid,
};

// ──────────────────────────────────────────────────────────────
// Primitive fragments
// ──────────────────────────────────────────────────────────────
static const std::unordered_map<std::string,std::string> g_primitiveMap = {
  {"INT",    "\\s*[-+]?\\d+\\s*"},
  {"DOUBLE", "\\s*[-+]?\\d+(?:\\.\\d+)?(?:[eE][-+]?\\d+)?\\s*"},
  {"STRING", "\\s*[^\\s]*[A-Za-z*][^\\s]*\\s*"},
  {"UINT",   "\\s*(?:0|[1-9]\\d*)\\s*"},
  {"BOOL",   "\\s*(?:true|false)\\s*"},
  // An index list: entries i, a-b or a-b:step, separated by commas (blanks
  // around a comma, '-' or ':' allowed); a list key's values ({...}) are
  // separated by blanks, so "13,14", "13, 14" and "13 14" are one list, as
  // parse_one_based_indices and parse_fixindex read them.
  {"INDEX_PATTERN",
   "\\s*[+]?\\d+(?:\\s*-\\s*[-+]?\\d+(?:\\s*:\\s*[+]?\\d+)?){0,1}"
   "(?:\\s*,\\s*[+]?\\d+(?:\\s*-\\s*[-+]?\\d+(?:\\s*:\\s*[+]?\\d+)?){0,1})*"
   "\\s*" }
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
                                     if(s=="DOUBLE")return 3;
                                     return 9999; };
  std::sort(alts.begin(),alts.end(),
            [&](const std::string&a,const std::string&b){return rank(a)<rank(b);});
  return alts;
}

// ──────────────────────────────────────────────────────────────
// Forward declarations
// ──────────────────────────────────────────────────────────────
static std::string buildTokenRegex(const std::string&);
static std::string buildRegexFromBlueprint(const std::string&,bool);

static bool isBlank(char c) { return std::isspace(static_cast<unsigned char>(c)) != 0; }

// ──────────────────────────────────────────────────────────────
// Lines of 2048 characters or more: checked without std::regex
// ──────────────────────────────────────────────────────────────
/* A line of numbers, split into values at blanks as the parser splits it.
 * kValid: every word is one number (strtod reads the whole word; an
 * underflow is a number, an overflow is not). kInvalid: every word is a
 * number or several numbers joined by commas, by a sign or directly
 * ('1.0,2.0', '1+2', '1-2', '1.5.2'), and one word is several: the parser
 * would keep such a word as one value. Otherwise kNotApplicable (a word
 * that is not numeric: the other checks decide). kValid takes no account
 * of the blueprint (a long numeric line of any list key), as before. */
static LongLineValidationResult validateLongNumericLine(const std::string& text)
{
  if (text.size() < 2048)
      return LongLineValidationResult::kNotApplicable;

  const char* p = text.c_str();
  bool sawAtLeastOne = false;
  bool joined = false;

  while (true)
  {
    while (*p && isBlank(*p)) ++p;
    if (*p == '\0') break;

    // One word: one number, or numbers joined by commas or a sign.
    bool first = true;
    while (*p && !isBlank(*p)) {
      if (*p == ',') {                  // never a separator of the parser
        joined = true;
        ++p;
        continue;
      }
      if (!first) joined = true;        // a number right after a number
      char* end = nullptr;
      errno = 0;
      const double v = std::strtod(p, &end);
      if (end == p)                     // not a number
        return LongLineValidationResult::kNotApplicable;
      // ERANGE: underflow (subnormal or flushed to 0, |v| < 1) is a valid
      // number; overflow (+-HUGE_VAL) is not.
      if (errno == ERANGE && !(std::fabs(v) < 1.0))
        return LongLineValidationResult::kNotApplicable;
      p = end;
      first = false;
    }
    sawAtLeastOne = true;
  }
  if (!sawAtLeastOne)
      return LongLineValidationResult::kNotApplicable;
  return joined ? LongLineValidationResult::kInvalid
                : LongLineValidationResult::kValid;
}

/* A line of a {STRING} list: words split at blanks, as the parser splits
 * them, each with a letter or a '*' (the STRING pattern; a word may hold a
 * comma or a '+', which the parser keeps in it). */
static LongLineValidationResult validateLongStringLine(const std::string& text)
{
  if (text.size() < 2048)
      return LongLineValidationResult::kNotApplicable;

  bool sawAtLeastOne = false;
  std::size_t pos = 0;

  while (pos < text.size()) {
    while (pos < text.size() && isBlank(text[pos])) {
      ++pos;
    }

    if (pos == text.size()) {
      break;
    }

    bool sawAlphaOrStar = false;
    while (pos < text.size() && !isBlank(text[pos])) {
      const unsigned char ch = static_cast<unsigned char>(text[pos]);
      if (std::isalpha(ch) || ch == '*') {
        sawAlphaOrStar = true;
      }
      ++pos;
    }

    if (!sawAlphaOrStar) {
      return LongLineValidationResult::kInvalid;
    }

    sawAtLeastOne = true;
  }

  return sawAtLeastOne ? LongLineValidationResult::kValid
                       : LongLineValidationResult::kInvalid;
}

/* A line of an {INDEX_PATTERN} list, by the grammar of the regex: entries
 * i, a-b or a-b:step ([+]digits, '-', [+-]digits, ':', [+]digits; blanks
 * around '-' and ':' allowed), separated by commas or blanks; no empty
 * entry, no leading or trailing comma. */
static LongLineValidationResult validateLongIndexLine(const std::string& text)
{
  if (text.size() < 2048)
      return LongLineValidationResult::kNotApplicable;

  // The text without the blanks next to '-' and ':', so that an entry is
  // one word.
  std::string list;
  list.reserve(text.size());
  char lastKept = '\0';  // the last non-blank character kept
  for (std::size_t i = 0; i < text.size(); ++i) {
    const char c = text[i];
    if (isBlank(c)) {
      std::size_t next = i;
      while (next < text.size() && isBlank(text[next])) ++next;
      const char after = next < text.size() ? text[next] : '\0';
      if (lastKept == '-' || lastKept == ':' || after == '-' || after == ':')
        continue;
    } else {
      lastKept = c;
    }
    list += c;
  }

  // [+]digits, or [+-]digits when `minus`; advances p.
  auto digits = [](const char*& p, const char* e, bool minus) {
    if (p < e && (*p == '+' || (minus && *p == '-'))) ++p;
    const char* b = p;
    while (p < e && std::isdigit(static_cast<unsigned char>(*p))) ++p;
    return p > b;
  };
  auto entry = [&](const char* p, const char* e) {
    if (!digits(p, e, false)) return false;
    if (p == e) return true;
    if (*p++ != '-' || !digits(p, e, true)) return false;
    if (p == e) return true;
    if (*p++ != ':' || !digits(p, e, false)) return false;
    return p == e;
  };

  std::size_t pos = 0;
  while (true) {
    const std::size_t comma = list.find(',', pos);
    const std::size_t stop = comma == std::string::npos ? list.size() : comma;
    bool sawEntry = false;
    std::size_t i = pos;
    while (i < stop) {
      while (i < stop && isBlank(list[i])) ++i;
      if (i == stop) break;
      const std::size_t b = i;
      while (i < stop && !isBlank(list[i])) ++i;
      if (!entry(list.data() + b, list.data() + i))
        return LongLineValidationResult::kInvalid;
      sawEntry = true;
    }
    if (!sawEntry)                      // an empty entry
      return LongLineValidationResult::kInvalid;
    if (comma == std::string::npos) break;
    pos = comma + 1;
  }
  return LongLineValidationResult::kValid;
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

    // The elements and the repetitions are separated by a blank, as the
    // parser splits a line (the primitives' own \\s* take more blanks).
    std::string block; bool first=true;
    for(const auto& s:subs){
      if(!first) block += "\\s";
      block += buildTokenRegex(s);
      first=false;
    }
    return "(?:"+block+")(?:\\s"+block+")*";
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

// ──────────────────────────────────────────────────────────────
// pImpl
// ──────────────────────────────────────────────────────────────
class BlueprintValidator::Impl {
public:
  explicit Impl(const std::vector<std::string>& pats,
                const std::vector<std::string>& blueprints)
    : onlyRepeatedStringBlueprint(
        std::all_of(blueprints.begin(), blueprints.end(),
                    [](const std::string& bp) { return trim(bp) == "{STRING}"; })),
      onlyIndexListBlueprint(
        !blueprints.empty() &&
        std::all_of(blueprints.begin(), blueprints.end(),
                    [](const std::string& bp) {
                      return trim(bp) == "{INDEX_PATTERN}";
                    })) {
    validateRegex.reserve(pats.size());
    for(auto& p:pats)
      validateRegex.emplace_back(p,
        std::regex::ECMAScript|std::regex::optimize|std::regex::nosubs);
  }
  ~Impl(){
    for(auto&r:validateRegex) std::regex{}.swap(r);
    std::vector<std::regex>().swap(validateRegex);
  }
  bool onlyRepeatedStringBlueprint;
  bool onlyIndexListBlueprint;
  std::vector<std::regex> validateRegex;
};

// ──────────────────────────────────────────────────────────────
// Constructors
// ──────────────────────────────────────────────────────────────
BlueprintValidator::BlueprintValidator(const std::string& bp)
{
  m_patterns = buildRegexFromBlueprint(bp,true);
  std::vector<std::string> nc { buildRegexFromBlueprint(bp,false) };
  m_impl = new Impl(nc, {bp});
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
  m_impl = new Impl(nonCap, bps);
}

// ──────────────────────────────────────────────────────────────
// validate()
// ──────────────────────────────────────────────────────────────
bool BlueprintValidator::validate(const std::string& text) const
{
  // Long lines are decided without std::regex (its recursion overflows the
  // stack on long input) where these checks can.
  if (m_impl && m_impl->onlyIndexListBlueprint) {
    const auto longIndexResult = validateLongIndexLine(text);
    if (longIndexResult != LongLineValidationResult::kNotApplicable)
      return longIndexResult == LongLineValidationResult::kValid;
  }

  const auto longNumericResult = validateLongNumericLine(text);
  if (longNumericResult == LongLineValidationResult::kValid)
      return true;
  if (longNumericResult == LongLineValidationResult::kInvalid)
      return false;

  if (m_impl && m_impl->onlyRepeatedStringBlueprint) {
    const auto longStringResult = validateLongStringLine(text);
    if (longStringResult == LongLineValidationResult::kValid)
      return true;
    if (longStringResult == LongLineValidationResult::kInvalid)
      return false;
  }

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
