#ifndef UTILS_H
#define UTILS_H

#include <tadah/core/core_types.h>

#include <iterator>
#include <algorithm>
#include <vector>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <random>
#include <tuple>
#include <cstddef>

namespace tadah {
namespace core {

/** Vector printing to streams */
template <typename T>
std::ostream& operator<< (std::ostream& os, const std::vector<T>& v) {
    if ( !v.empty() ) {
        //os << '[';
        std::copy (v.begin(), v.end(), std::ostream_iterator<T>(os, " "));
        //os << "\b\b]\n";
    }
    return os;
}

template<typename T>
class Logspace {
    private:
        T curValue, base;

    public:
        Logspace(T first, T base) : curValue(first), base(base) {}

        T operator()() {
            T retval = curValue;
            curValue *= base;
            return retval;
        }
};

v_type logspace(double start, double stop, int num, double base);
template<typename T>
v_type linspace(T start_in, T end_in, int num_in)
{

  v_type ls(num_in);

  double start = static_cast<double>(start_in);
  double end = static_cast<double>(end_in);
  double num = static_cast<double>(num_in);

  if (num == 0) { return ls; }
  if (num == 1)
    {
      ls[0] = start;
      return ls;
    }

  double delta = (end - start) / (num - 1);

  for(int i=0; i < num-1; ++i)
    {
      //ls.push_back(start + delta * i);
      ls[i] = start + delta * i;
    }
  ls[num_in-1] = end;
  return ls;
}


/**
 * @brief Convert a value to string (no precision specified).
 *
 * @tparam T Any type supporting operator<< to std::ostream.
 * @param value The value to convert.
 * @return std::string Representation of the value.
 */
template <typename T>
inline std::string to_string(const T& value)
{
    std::stringstream ss;
    ss << value; 
    return ss.str();
}

/**
 * @brief Convert a value to string with given precision (for numeric types).
 *
 * @tparam T Any type supporting operator<< to std::ostream.
 * @param value The value to convert.
 * @param n   Decimal precision (for floating types).
 * @return std::string Representation of the value with precision n.
 */
template <typename T>
inline std::string to_string(const T& value, int n)
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(n) << value;
    return ss.str();
}

/**
 * @brief Specialization for bool (no precision).
 *
 * Writes "true" or "false" instead of "1" or "0".
 */
template <>
inline std::string to_string<bool>(const bool& value)
{
    return value ? "true" : "false";
}

/**
 * @brief Specialization for bool (with precision).
 *
 * Ignores the precision parameter and writes "true" or "false".
 */
template <>
inline std::string to_string<bool>(const bool& value, int /*n*/)
{
    return value ? "true" : "false";
}


std::string to_upper(const std::string s);

template <typename F>
double central_difference(F f, double val, double h) {
    return (f(val+h)-f(val-h))/(2.0*h);
}
size_t number_of_digits (size_t i);
size_t max_number(size_t i);    // max_number(6) -> 999999

template<typename T>
T random(T range_from, T range_to) {
    std::random_device                  rand_dev;
    std::mt19937                        generator(rand_dev());
    std::uniform_real_distribution<T>    distr(range_from, range_to);
    return distr(generator);
}

/** Find element in std::vector and return its index.
 *
 * If not found returns -1.
 */
template <typename T>
int get_index(std::vector<T> v, T K) {
    auto it = find(v.begin(), v.end(), K);
    int index=-1;
    if (it != v.end()) {
        index = it - v.begin();
    }
    return index;
}
std::string vec_to_string(std::vector<double> vec,
        std::string delim=" ");

/** Print C++ parameter pack
 *
 * Example:
 *  print_pp(std::cout,args...);
 */
template <typename Arg, typename... Args>
void print_pp(std::ostream& out, Arg&& arg, Args&&... args)
{
    out << std::forward<Arg>(arg);
    using expander = int[];
    (void)expander{0, (void(out << ',' << std::forward<Args>(args)), 0)...};
}

//std::vector<double> calc_rho(Structure &st);

bool is_blank_line(const std::string& line);

/**
 * @brief Checks if a line is blank (empty or only contains whitespace).
 * 
 * @param line The string to check.
 * @return true if the line is blank, false otherwise.
 */
bool is_blank_line(const std::string& line);

/**
 * @brief Divides a number into approximately equal parts.
 * 
 * @param number The number to divide.
 * @param n The number of parts to divide into.
 * @return A vector containing the parts.
 */
std::vector<size_t> divide_into_parts(size_t number, int n);

/**
 * @brief Divides a number according to specified percentages.
 * 
 * @param number The total number to divide.
 * @param percentages A vector of percentages to divide by.
 * @return A vector containing the divided parts based on percentages.
 */
std::vector<size_t> divide_by_percentage(size_t number, const std::vector<size_t>& percentages);

/**
 * @brief Checks if a double represents an integer.
 * 
 * @param d The double to check.
 * @return true if the double represents an integer.
 */
bool is_integer(double d);

/**
 * @brief Checks if a string represents an integer.
 * 
 * @param s The string to check.
 * @return true if the string is an integer.
 */
bool is_integer(const std::string& s);

/**
 * @brief Checks if a string represents a number.
 * 
 * @param s The string to check.
 * @return true if the string is numeric, false otherwise.
 */
bool is_number(const std::string& s);

/**
 * @brief Trims leading and trailing spaces from a string.
 * 
 * @param str The string to trim.
 * @return A new string with leading and trailing spaces removed.
 */
std::string trim(const std::string& str);

/**
 * @brief Combines a vector of strings into a single string, with each element separated by a space.
 * 
 * @param vec The vector of strings to join.
 * @return A single string containing all elements of the vector, separated by spaces.
 */
std::string join(const std::vector<std::string>& vec);

/**
 * @brief Parses a string of indices, handling ranges and steps.
 * 
 * @param input The string containing indices to parse.
 * @return A vector of unique indices extracted from the string.
 */
std::vector<size_t> parse_indices(const std::string& input);


/**
 * @brief Parses a vector of strings containing raw indices from user configuration, handling ranges and steps.
 * 
 * @param input The vector of strings containing indices to parse.
 * @return A vector of unique indices extracted from the input.
 */
std::vector<size_t> parse_indices(const std::vector<std::string>& input);


/**
 * @brief Move specified columns in place to the right end of the matrix.
 *
 * This function rearranges the columns of the matrix such that the columns
 * specified by indices are moved to the right, and returns a mapping
 * of the original index to the new index.
 *
 * @param matrix The matrix whose columns are to be rearranged.
 * @param indices A vector of column indices to move to the right.
 * @return A vector mapping original column indices to their new positions.
 */
template <typename T>
std::vector<size_t> move_columns_in_place(Matrix_Base<T>& matrix, const std::vector<size_t>& indices) {
  size_t N = matrix.cols();
  size_t num_indices = indices.size();
  size_t boundary = N - num_indices;

  std::vector<size_t> column_map(N);
  for (size_t i = 0; i < N; ++i) {
    column_map[i] = i;
  }

  size_t insertPos = 0;

  for (size_t i = 0; i < N; ++i) {
    if (insertPos < boundary && 
      std::find(indices.begin(), indices.end(), i) == indices.end()) {
      matrix.swap_columns(insertPos, i);

      std::swap(column_map[insertPos], column_map[i]);

      insertPos++;
    }
  }

  return column_map;
}

/**
 * @brief Reverses the column arrangement of the matrix using a given column map.
 *
 * The function restores the original column order of the matrix based on a mapping
 * that was used to rearrange the columns initially.
 * 
 * @param matrix The matrix that needs its columns restored to the original order.
 * @param column_map A vector mapping current column indices to their original positions.
 */
template <typename T>
void reverse_columns(Matrix_Base<T> &matrix, const std::vector<size_t>& column_map) {
    std::vector<size_t> temp_map = column_map;
    for (size_t i = 0; i < temp_map.size(); ++i) {
        while (temp_map[i] != i) {
            size_t original_pos = temp_map[i];
            matrix.swap_columns(i, original_pos);
            std::swap(temp_map[i], temp_map[original_pos]);
        }
    }
}

/**
 * @brief Reverses a vector arrangement using a given map.
 *
 * @param vec The vector to restore.
 * @param original_map A mapping of the rearranged indices to original positions.
 */
void reverse_vector(t_type& vec, const std::vector<size_t>& original_map);

/**
 * @typedef histogram_bin
 * @brief Type alias for a histogram bin containing the bin center coordinates and the count.
 *
 * Each histogram bin is represented as a tuple containing:
 * - `double x_center`: The center coordinate of the bin along the x-axis.
 * - `double y_center`: The center coordinate of the bin along the y-axis.
 * - `size_t count`: The number of data points that fall into the bin.
 */
typedef std::tuple<double, double, size_t> histogram_bin;

/**
 * @brief Generates a 2D histogram from input data.
 *
 * This function takes a 2D array of doubles representing (x, y) data points and generates
 * a 2D histogram. The histogram is returned as a vector of tuples, where each tuple contains
 * the x and y coordinates of the bin center and the count of data points within that bin.
 * The minimum and maximum values along each axis are calculated from the input data.
 *
 * @param data_2d A vector containing the (x, y) data points.
 * @param x_bins The number of bins along the x-axis.
 * @param y_bins The number of bins along the y-axis.
 * @return A vector of histogram bins, each containing the bin center coordinates and the count.
 */
std::vector<histogram_bin> generate_2d_histogram(
    const std::vector<std::pair<double,double>>& data_2d,
    int x_bins,
    int y_bins);
} // namespace core
} // namespace tadah
#endif
