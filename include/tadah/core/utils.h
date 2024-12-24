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




template <typename T>
std::string to_string(const T& value,int n) {
    std::stringstream ss;
    ss <<  std::setprecision(n) << value;
    return ss.str();
}

template <typename T>
std::string to_string(const T& value) {
    std::stringstream ss;
    ss << value;
    return ss.str();
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
std::vector<size_t> divide_into_parts(size_t number, size_t n);

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
 * @brief Parses a string of indices, handling ranges and steps.
 * 
 * @param input The string containing indices to parse.
 * @return A vector of unique indices extracted from the string.
 */
std::vector<size_t> parse_indices(const std::string& input);

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

#endif
