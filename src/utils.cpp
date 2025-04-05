#include <tadah/core/utils.h>
#include <stdexcept>
#include <set>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>

namespace tadah {
namespace core {

v_type logspace(double start, double stop, int num, double base) {
    // Disallow any non-positive start to avoid std::log of negative or zero
    if (start <= 0.0) {
        throw std::runtime_error("logspace() start must be > 0");
    }

    // existing code
    double realStart = std::pow(base, std::log(start));
    double realBase  = std::pow(base, (std::log(stop)-std::log(start)) / (num - 1));

    v_type retval;
    retval.reserve(num);
    std::generate_n(std::back_inserter(retval), num, Logspace<double>(realStart, realBase));
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
std::string to_upper(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), ::toupper);
  return s;
}
std::string to_lower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), ::tolower);
  return s;
}
bool is_blank_line(const std::string& line) {
    for (char c : line) {
        if (!std::isspace(static_cast<unsigned char>(c))) {
            return false;
        }
    }
    return true;
}


std::vector<size_t> divide_into_parts(size_t number, int n) {
  if (n <= 0) {
    throw std::runtime_error("divide_into_parts called with n=0");
  }
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
    const std::string whitespace = " \t\n\r\f\v";
    size_t first = str.find_first_not_of(whitespace);
    if (first == std::string::npos) {
        return "";
    }
    size_t last = str.find_last_not_of(whitespace);
    return str.substr(first, last - first + 1);
}

void trimInPlace(std::string& str) {
    const std::string whitespace = " \t\n\r\f\v";
    size_t first = str.find_first_not_of(whitespace);
    if (first == std::string::npos) {
        str.clear();
        return;
    }
    size_t last = str.find_last_not_of(whitespace);
    str = str.substr(first, last - first + 1);
}

std::string join(const std::vector<std::string>& vec) {
    std::ostringstream oss;
    if (!vec.empty()) {
        oss << vec[0];
        for (size_t i = 1; i < vec.size(); ++i) {
            oss << ' ' << vec[i];
        }
    }
    return oss.str();
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
std::vector<size_t> parse_indices(const std::vector<std::string>& input) {
  std::string input_str = join(input);
  return parse_indices(input_str);
}

void reverse_vector(t_type& vec, const std::vector<size_t>& original_map) {
    t_type temp_vec = vec;
    for (size_t i = 0; i < original_map.size(); ++i) {
        vec[original_map[i]] = temp_vec[i];
    }
}

std::vector<histogram_bin> generate_2d_histogram(
    const std::vector<std::pair<double,double>>& data_2d,
    int x_bins,
    int y_bins)
{
    if (data_2d.empty()) {
        return {};
    }

    // Initialize minimum and maximum values
    double x_min = std::numeric_limits<double>::max();
    double x_max = std::numeric_limits<double>::lowest();
    double y_min = std::numeric_limits<double>::max();
    double y_max = std::numeric_limits<double>::lowest();

    // Find min and max values along x and y axes from the data
    for (const auto& point : data_2d) {
        double x_value = point.first;
        double y_value = point.second;

        if (x_value < x_min) x_min = x_value;
        if (x_value > x_max) x_max = x_value;
        if (y_value < y_min) y_min = y_value;
        if (y_value > y_max) y_max = y_value;
    }

    // Check if min and max values are valid
    if (x_min >= x_max || y_min >= y_max) {
        // Invalid data range
        return {};
    }

    // Initialize the histogram bins with zero counts
    std::vector<std::vector<size_t>> histogram(x_bins, std::vector<size_t>(y_bins, 0));

    // Calculate the width of each bin along x and y axes
    double x_bin_width = (x_max - x_min) / x_bins;
    double y_bin_width = (y_max - y_min) / y_bins;

    // Iterate over the data points and increment the corresponding histogram bins
    for (const auto& point : data_2d) {
        double x_value = point.first;
        double y_value = point.second;

        // Calculate the bin indices for the current data point
        int x_bin = static_cast<int>((x_value - x_min) / x_bin_width);
        int y_bin = static_cast<int>((y_value - y_min) / y_bin_width);

        // Handle the edge case where the data point is exactly at the maximum value
        if (x_bin == x_bins) x_bin = x_bins - 1;
        if (y_bin == y_bins) y_bin = y_bins - 1;

        // Increment the count in the appropriate bin
        ++histogram[x_bin][y_bin];
    }

    // Prepare the output vector containing histogram bins with centers and counts
    std::vector<histogram_bin> histogram_bins;
    for (int ix = 0; ix < x_bins; ++ix) {
        double x_center = x_min + (ix + 0.5) * x_bin_width;
        for (int iy = 0; iy < y_bins; ++iy) {
            double y_center = y_min + (iy + 0.5) * y_bin_width;
            size_t count = histogram[ix][iy];
            histogram_bins.push_back(std::make_tuple(x_center, y_center, count));
        }
    }

    return histogram_bins;
}

std::string removeFirstWord(const std::string &input, const std::string &knownWord) {
  // Check whether 'input' starts with 'knownWord'
  if (input.compare(0, knownWord.size(), knownWord) == 0) {
    // Compute new starting position after removing the known word
    // plus any subsequent whitespace
    std::size_t startPos = knownWord.size();
    while (startPos < input.size() && std::isspace(static_cast<unsigned char>(input[startPos]))) {
      ++startPos;
    }
    // Return the substring excluding the known word and trailing spaces
    return input.substr(startPos);
  }
  // Return original string if the word does not appear at the start
  return input;
}

std::string concat(const std::vector<std::string>& vec, const std::string& delim) {
  std::string result;
  for (const auto& str : vec) {
    if (!result.empty()) {
      result += delim;
    }
    result += str;
  }
  return result;
}

} // namespace core
} // namespace tadah

