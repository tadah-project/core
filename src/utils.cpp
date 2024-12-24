#include <tadah/core/utils.h>
#include <stdexcept>
#include <set>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>

v_type logspace(double start, double stop, int num, double base) {
    if (start==0) {
        throw std::runtime_error("log series cannot start at 0\n");
    }
    double realStart = pow(base, log(start));
    double realBase = pow(base, (log(stop)-log(start))/(num-1));

    v_type retval;
    retval.reserve(num);
    std::generate_n(std::back_inserter(retval), num, Logspace<double>(realStart,realBase));
    return retval;
}

size_t number_of_digits (size_t i)
{
    return i > 0 ? (int) log10 ((double) i) + 1 : 1;
}
size_t max_number(size_t n) {
    size_t val = 0;
    if (n == 0) return 0;
    for (size_t i=0; i<n; ++i) val+= 9*std::pow(10,i);
    return val;
}

// convert vector to string
std::string vec_to_string(std::vector<double> vec,
        std::string delim) {
    std::stringstream ss;
    for (auto &v:vec) ss << std::setprecision(15)
        << v << delim;
    return ss.str().substr(0,ss.str().size()-delim.size()); // remove last delim
}

//std::vector<double> calc_rho(Structure &st) {
//    // Computes density according to Ta2 by Ravelo
//    double p=5.9913;
//    double q=8.0;
//    double rho0=0.074870;
//    double rc=5.5819;
//    double rc_sq=rc*rc;
//    double rcp=pow(rc,p);
//    double a0=3.304;
//    double r0=sqrt(3)*a0/2;
//    double r0p=pow(r0,p);
//    // assume full NN list
//    std::vector<double> rho(st.natoms(),0);
//    for (size_t i=0; i<st.natoms(); ++i) {
//        const Atom &a1 = st.atoms[i];
//        for (size_t jj=0; jj<st.nn_size(i); ++jj) {
//            const Vec3d &a2pos = st.nn_pos(i,jj);
//            Vec3d delij = a1.position - a2pos;
//            double rij_sq = delij.transpose() * delij;
//            if (rij_sq > rc_sq) continue;
//            double r=sqrt(rij_sq);
//            rho[i]+=rho0*pow((rcp-pow(r,p))/(rcp-r0p),q);
//        }
//    }
//    return rho;
//}
std::string to_upper(const std::string s) {
    std::string str = s;
    std::transform(str.begin(), str.end(),str.begin(), ::toupper);
    return str;
}
bool is_blank_line(const std::string& line) {
    for (char c : line) {
        if (!std::isspace(static_cast<unsigned char>(c))) {
            return false;
        }
    }
    return true;
}


std::vector<size_t> divide_into_parts(size_t number, size_t n) {
    std::vector<size_t> parts(n, number / n);
    size_t remainder = number % n;
    for (size_t i = 0; i < remainder; ++i) {
        parts[i] += 1;
    }
    return parts;
}

std::vector<size_t> divide_by_percentage(size_t number, const std::vector<size_t>& percentages) {
    std::vector<size_t> parts(percentages.size(), 0);
    size_t totalAssigned = 0;

    for (size_t i = 0; i < percentages.size(); ++i) {
        parts[i] = number * percentages[i] / 100;
        totalAssigned += parts[i];
    }

    size_t remainder = number - totalAssigned;

    // Distribute remaining amount due to rounding
    for (size_t i = 0; remainder > 0; ++i, --remainder) {
        parts[i]++;
    }

    return parts;
}
bool is_integer(double d) {
    return std::floor(d) == d;
}
bool is_integer(const std::string& s) {
    return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
}
bool is_number(const std::string& str) {
    if (str.empty()) return false;

    try {
        size_t pos;
        std::stod(str, &pos);
        return pos == str.size();
    } catch (std::invalid_argument& e) {
        return false;
    } catch (std::out_of_range& e) {
        return false;
    }
}

std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(' ');
    if (first == std::string::npos)
        return "";
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

std::vector<size_t> parse_indices(const std::string& input) {
    std::vector<size_t> result;
    std::set<size_t> uniqueNumbers;
    std::stringstream ss(input);
    std::string segment;
    bool hasDuplicates = false;

    while (std::getline(ss, segment, ',')) {
        segment = trim(segment);
        size_t dashPos = segment.find('-');
        size_t colonPos = segment.find(':');

        if (dashPos == std::string::npos) {
            // Single number
            if (is_integer(segment)) {
                size_t number = static_cast<size_t>(std::stoul(segment));
                if (!uniqueNumbers.insert(number).second) {
                    hasDuplicates = true;
                } else {
                    result.push_back(number);
                }
            } else {
                std::cerr << "Error: Non-numeric value or incorrect syntax found: " << segment << std::endl;
            }
        } else {
            // Extract range start
            std::string startStr = trim(segment.substr(0, dashPos));
            if (!is_integer(startStr)) {
                std::cerr << "Error: Non-numeric start value or incorrect syntax found: " << startStr << std::endl;
                continue;
            }
            size_t start = static_cast<size_t>(std::stoul(startStr));

            size_t end, step = 1;  // Default step is 1

            if (colonPos != std::string::npos) {
                std::string endStr = trim(segment.substr(dashPos + 1, colonPos - dashPos - 1));
                std::string stepStr = trim(segment.substr(colonPos + 1));
                if (!is_integer(endStr) || !is_integer(stepStr)) {
                    std::cerr << "Error: Non-numeric end or step value or incorrect syntax found." << std::endl;
                    continue;
                }
                end = static_cast<size_t>(std::stoul(endStr));
                step = static_cast<size_t>(std::stoul(stepStr));
            } else if (dashPos + 1 < segment.size()) {
                std::string endStr = trim(segment.substr(dashPos + 1));
                if (!is_integer(endStr)) {
                    std::cerr << "Error: Non-numeric end value or incorrect syntax found: " << endStr << std::endl;
                    continue;
                }
                end = static_cast<size_t>(std::stoul(endStr));
            } else {
                if (!uniqueNumbers.insert(start).second) {
                    hasDuplicates = true;
                } else {
                    result.push_back(start);
                }
                continue;
            }

            // Generate numbers in range [start, end] with given step
            for (size_t i = start; i <= end; i += step) {
                if (!uniqueNumbers.insert(i).second) {
                    hasDuplicates = true;
                } else {
                    result.push_back(i);
                }
            }
        }
    }

    if (hasDuplicates) {
        std::cerr << "Warning: Duplicate indices found and removed." << std::endl;
    }

    return result;
}

void reverse_vector(t_type& vec, const std::vector<size_t>& original_map) {
    t_type temp_vec = vec;
    for (size_t i = 0; i < original_map.size(); ++i) {
        vec[original_map[i]] = temp_vec[i];
    }
}
