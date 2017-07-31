// ---------------------------------------------------------------------
//
// Copyright (C) 1998 - 2017 by the deal.II authors
//
// This file is part of the deal.II library.
//
// The deal.II library is free software; you can use it, redistribute
// it, and/or modify it under the terms of the GNU Lesser General
// Public License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// The full text of the license can be found in the file LICENSE at
// the top level of the deal.II distribution.
//
// ---------------------------------------------------------------------

#ifndef dealii__patterns_h
#define dealii__patterns_h


#include <deal.II/base/config.h>
#include <deal.II/base/exceptions.h>
#include <deal.II/base/subscriptor.h>
#include <deal.II/base/point.h>
#include <deal.II/base/std_cxx14/memory.h>
#include <deal.II/base/subscriptor.h>
#include <deal.II/base/utilities.h>
#include <deal.II/base/std_cxx14/algorithm.h>

#include <boost/archive/basic_archive.hpp>
#include <boost/core/demangle.hpp>
#include <boost/property_tree/ptree_fwd.hpp>
#include <boost/property_tree/ptree_serialization.hpp>
#include <boost/serialization/split_member.hpp>

#include <array>
#include <deque>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

DEAL_II_NAMESPACE_OPEN

// forward declarations for interfaces and friendship
class LogStream;
class MultipleParameterLoop;

/**
 * Namespace for a few classes that act as patterns for the ParameterHandler
 * class. These classes implement an interface that checks whether a parameter
 * in an input file matches a certain pattern, such as "being boolean", "an
 * integer value", etc.
 *
 * @ingroup input
 */
namespace Patterns
{

  /**
   * Base class to declare common interface. The purpose of this class is
   * mostly to define the interface of patterns, and to force derived classes
   * to have a <tt>clone</tt> function. It is thus, in the languages of the
   * "Design Patterns" book (Gamma et al.), a "prototype".
   */
  class PatternBase
  {
  public:
    /**
     * Make destructor of this and all derived classes virtual.
     */
    virtual ~PatternBase ();

    /**
     * Return <tt>true</tt> if the given string matches the pattern.
     */
    virtual bool match (const std::string &test_string) const = 0;

    /**
     * List of possible description output formats.
     *
     * Capitalization chosen for similarity to ParameterHandler::OutputStyle.
     */
    enum OutputStyle
    {
      /**
       * Simple text suitable for machine parsing in the static public member
       * functions for all of the built in inheriting classes.
       *
       * Preferably human readable, but machine parsing is more critical.
       */
      Machine,
      /**
       * Easily human readable plain text format suitable for plain text
       * documentation.
       */
      Text,
      /**
       * Easily human readable LaTeX format suitable for printing in manuals.
       */
      LaTeX
    };

    /**
     * Return a string describing the pattern.
     */
    virtual std::string description (const OutputStyle style=Machine) const = 0;

    /**
     * Return a pointer to an exact copy of the object. This is necessary
     * since we want to store objects of this type in containers, were we need
     * to copy objects without knowledge of their actual data type (we only
     * have pointers to the base class).
     *
     * Ownership of the objects returned by this function is passed to the
     * caller of this function.
     */
    virtual std::unique_ptr<PatternBase> clone () const = 0;

    /**
     * Determine an estimate for the memory consumption (in bytes) of this
     * object. To avoid unnecessary overhead, we do not force derived classes
     * to provide this function as a virtual overloaded one, but rather try to
     * cast the present object to one of the known derived classes and if that
     * fails then take the size of this base class instead and add 32 byte
     * (this value is arbitrary, it should account for virtual function
     * tables, and some possible data elements). Since there are usually not
     * many thousands of objects of this type around, and since the
     * memory_consumption mechanism is used to find out where memory in the
     * range of many megabytes is, this seems like a reasonable approximation.
     *
     * On the other hand, if you know that your class deviates from this
     * assumption significantly, you can still overload this function.
     */
    virtual std::size_t memory_consumption () const;
  };

  /**
   * Return pointer to the correct derived class based on description.
   */
  std::unique_ptr<PatternBase> pattern_factory (const std::string &description);

  /**
   * Test for the string being an integer. If bounds are given to the
   * constructor, then the integer given also needs to be within the interval
   * specified by these bounds. Note that unlike common convention in the C++
   * standard library, both bounds of this interval are inclusive; the reason
   * is that in practice in most cases, one needs closed intervals, but these
   * can only be realized with inclusive bounds for non-integer values. We
   * thus stay consistent by always using closed intervals.
   *
   * If the upper bound given to the constructor is smaller than the
   * lower bound, then every integer is allowed.
   *
   * Giving bounds may be useful if for example a value can only be positive
   * and less than a reasonable upper bound (for example the number of
   * refinement steps to be performed), or in many other cases.
   */
  class Integer : public PatternBase
  {
  public:
    /**
     * Minimal integer value. If the numeric_limits class is available use
     * this information to obtain the extremal values, otherwise set it so
     * that this class understands that all values are allowed.
     */
    static const int min_int_value;

    /**
     * Maximal integer value. If the numeric_limits class is available use
     * this information to obtain the extremal values, otherwise set it so
     * that this class understands that all values are allowed.
     */
    static const int max_int_value;

    /**
     * Constructor. Bounds can be specified within which a valid
     * parameter has to be. If the upper bound is smaller than the
     * lower bound, then the entire set of integers is implied. The
     * default values are chosen such that no bounds are enforced on
     * parameters.
     */
    Integer (const int lower_bound = min_int_value,
             const int upper_bound = max_int_value);

    /**
     * Return <tt>true</tt> if the string is an integer and its value is
     * within the specified range.
     */
    virtual bool match (const std::string &test_string) const;

    /**
     * Return a description of the pattern that valid strings are expected to
     * match. If bounds were specified to the constructor, then include them
     * into this description.
     */
    virtual std::string description (const OutputStyle style=Machine) const;

    /**
     * Return a copy of the present object, which is newly allocated on the
     * heap. Ownership of that object is transferred to the caller of this
     * function.
     */
    virtual std::unique_ptr<PatternBase> clone () const;

    /**
     * Creates new object if the start of description matches
     * description_init.  Ownership of that object is transferred to the
     * caller of this function.
     */
    static std::unique_ptr<Integer> create (const std::string &description);

  private:
    /**
     * Value of the lower bound. A number that satisfies the
     * @ref match
     * operation of this class must be equal to this value or larger, if the
     * bounds of the interval for a valid range.
     */
    const int lower_bound;

    /**
     * Value of the upper bound. A number that satisfies the
     * @ref match
     * operation of this class must be equal to this value or less, if the
     * bounds of the interval for a valid range.
     */
    const int upper_bound;

    /**
     * Initial part of description
     */
    static const char *description_init;
  };

  /**
   * Test for the string being a <tt>double</tt>. If bounds are given to the
   * constructor, then the integer given also needs to be within the interval
   * specified by these bounds. Note that unlike common convention in the C++
   * standard library, both bounds of this interval are inclusive; the reason
   * is that in practice in most cases, one needs closed intervals, but these
   * can only be realized with inclusive bounds for non-integer values. We
   * thus stay consistent by always using closed intervals.
   *
   * If the upper bound given to the constructor is smaller than the
   * lower bound, then every double precision number is allowed.
   *
   * Giving bounds may be useful if for example a value can only be positive
   * and less than a reasonable upper bound (for example damping parameters
   * are frequently only reasonable if between zero and one), or in many other
   * cases.
   */
  class Double : public PatternBase
  {
  public:
    /**
     * Minimal double value used as default value, taken from
     * <tt>std::numeric_limits</tt>.
     */
    static const double min_double_value;

    /**
     * Maximal double value used as default value, taken from
     * <tt>std::numeric_limits</tt>.
     */
    static const double max_double_value;

    /**
     * Constructor. Bounds can be specified within which a valid
     * parameter has to be. If the upper bound is smaller than the
     * lower bound, then the entire set of double precision numbers is
     * implied. The default values are chosen such that no bounds are
     * enforced on parameters.
     */
    Double (const double lower_bound = min_double_value,
            const double upper_bound = max_double_value);

    /**
     * Return <tt>true</tt> if the string is a number and its value is within
     * the specified range.
     */
    virtual bool match (const std::string &test_string) const;

    /**
     * Return a description of the pattern that valid strings are expected to
     * match. If bounds were specified to the constructor, then include them
     * into this description.
     */
    virtual std::string description (const OutputStyle style=Machine) const;

    /**
     * Return a copy of the present object, which is newly allocated on the
     * heap. Ownership of that object is transferred to the caller of this
     * function.
     */
    virtual std::unique_ptr<PatternBase> clone () const;

    /**
     * Creates a new object on the heap using @p new if the given
     * @p description is a valid format (for example created by calling
     * description() on an existing object), or @p nullptr otherwise. Ownership
     * of the returned object is transferred to the caller of this function,
     * which should be freed using @p delete.
     */
    static std::unique_ptr<Double> create(const std::string &description);

  private:
    /**
     * Value of the lower bound. A number that satisfies the
     * @ref match
     * operation of this class must be equal to this value or larger, if the
     * bounds of the interval form a valid range.
     */
    const double lower_bound;

    /**
     * Value of the upper bound. A number that satisfies the
     * @ref match
     * operation of this class must be equal to this value or less, if the
     * bounds of the interval form a valid range.
     */
    const double upper_bound;

    /**
     * Initial part of description
     */
    static const char *description_init;
  };

  /**
   * Test for the string being one of a sequence of values given like a
   * regular expression. For example, if the string given to the constructor
   * is <tt>"red|blue|black"</tt>, then the
   * @ref match
   * function returns <tt>true</tt> exactly if the string is either "red" or
   * "blue" or "black". Spaces around the pipe signs do not matter and are
   * eliminated.
   */
  class Selection : public PatternBase
  {
  public:
    /**
     * Constructor. Take the given parameter as the specification of valid
     * strings.
     */
    Selection (const std::string &seq);

    /**
     * Return <tt>true</tt> if the string is an element of the description
     * list passed to the constructor.
     */
    virtual bool match (const std::string &test_string) const;

    /**
     * Return a description of the pattern that valid strings are expected to
     * match. Here, this is the list of valid strings passed to the
     * constructor.
     */
    virtual std::string description (const OutputStyle style=Machine) const;

    /**
     * Return a copy of the present object, which is newly allocated on the
     * heap. Ownership of that object is transferred to the caller of this
     * function.
     */
    virtual std::unique_ptr<PatternBase> clone () const;

    /**
     * Determine an estimate for the memory consumption (in bytes) of this
     * object.
     */
    std::size_t memory_consumption () const;

    /**
     * Creates new object if the start of description matches
     * description_init.  Ownership of that object is transferred to the
     * caller of this function.
     */
    static std::unique_ptr<Selection> create (const std::string &description);

  private:
    /**
     * List of valid strings as passed to the constructor. We don't make this
     * string constant, as we process it somewhat in the constructor.
     */
    std::string sequence;

    /**
     * Initial part of description
     */
    static const char *description_init;
  };


  /**
   * This pattern matches a list of values separated by commas (or another
   * string), each of which have to match a pattern given to the constructor.
   * With two additional parameters, the number of elements this list has to
   * have can be specified. If none is specified, the list may have zero or
   * more entries.
   */
  class List : public PatternBase
  {
  public:
    /**
     * Maximal integer value. If the numeric_limits class is available use
     * this information to obtain the extremal values, otherwise set it so
     * that this class understands that all values are allowed.
     */
    static const unsigned int max_int_value;

    /**
     * Constructor. Take the given parameter as the specification of valid
     * elements of the list.
     *
     * The three other arguments can be used to denote minimal and maximal
     * allowable lengths of the list, and the string that is used as a
     * separator between elements of the list.
     */
    List (const PatternBase  &base_pattern,
          const unsigned int  min_elements = 0,
          const unsigned int  max_elements = max_int_value,
          const std::string  &separator = ",");


    /**
     * Return the internally stored separator.
     */
    const std::string &get_separator() const;

    /**
     * Return the internally stored base pattern.
     */
    const PatternBase &get_base_pattern() const;

    /**
     * Copy constructor.
     */
    List (const List &other);

    /**
     * Return <tt>true</tt> if the string is a comma-separated list of strings
     * each of which match the pattern given to the constructor.
     */
    virtual bool match (const std::string &test_string) const;

    /**
     * Return a description of the pattern that valid strings are expected to
     * match.
     */
    virtual std::string description (const OutputStyle style=Machine) const;

    /**
     * Return a copy of the present object, which is newly allocated on the
     * heap. Ownership of that object is transferred to the caller of this
     * function.
     */
    virtual std::unique_ptr<PatternBase> clone () const;

    /**
     * Creates new object if the start of description matches
     * description_init.  Ownership of that object is transferred to the
     * caller of this function.
     */
    static std::unique_ptr<List> create (const std::string &description);

    /**
     * Determine an estimate for the memory consumption (in bytes) of this
     * object.
     */
    std::size_t memory_consumption () const;

    /**
     * @addtogroup Exceptions
     * @{
     */

    /**
     * Exception.
     */
    DeclException2 (ExcInvalidRange,
                    int, int,
                    << "The values " << arg1 << " and " << arg2
                    << " do not form a valid range.");
    //@}
  private:
    /**
     * Copy of the pattern that each element of the list has to satisfy.
     */
    std::unique_ptr<PatternBase> pattern;

    /**
     * Minimum number of elements the list must have.
     */
    const unsigned int min_elements;

    /**
     * Maximum number of elements the list must have.
     */
    const unsigned int max_elements;

    /**
     * Separator between elements of the list.
     */
    const std::string separator;

    /**
     * Initial part of description
     */
    static const char *description_init;
  };


  /**
   * This pattern matches a list of comma-separated values each of which
   * denotes a pair of key and value. Both key and value have to match a
   * pattern given to the constructor. For each entry of the map, parameters
   * have to be entered in the form <code>key: value</code>. In other words, a
   * map is described in the form <code>key1: value1, key2: value2, key3:
   * value3, ...</code>. Two constructor arguments allow to choose a delimiter
   * between pairs other than the comma, and a delimeter between key and value
   * other than column.
   *
   * With two additional parameters, the number of elements this list has to
   * have can be specified. If none is specified, the map may have zero or
   * more entries.
   */
  class Map : public PatternBase
  {
  public:
    /**
     * Maximal integer value. If the numeric_limits class is available use
     * this information to obtain the extremal values, otherwise set it so
     * that this class understands that all values are allowed.
     */
    static const unsigned int max_int_value;

    /**
     * Constructor. Take the given parameter as the specification of valid
     * elements of the list.
     *
     * The four other arguments can be used to denote minimal and maximal
     * allowable lengths of the list as well as the separators used to delimit
     * pairs of the map and the symbol used to separate keys and values.
     */
    Map (const PatternBase  &key_pattern,
         const PatternBase  &value_pattern,
         const unsigned int  min_elements = 0,
         const unsigned int  max_elements = max_int_value,
         const std::string  &separator = ",",
         const std::string  &key_value_separator = ":");

    /**
     * Copy constructor.
     */
    Map (const Map &other);

    /**
     * Return <tt>true</tt> if the string is a comma-separated list of strings
     * each of which match the pattern given to the constructor.
     */
    virtual bool match (const std::string &test_string) const;

    /**
     * Return a description of the pattern that valid strings are expected to
     * match.
     */
    virtual std::string description (const OutputStyle style=Machine) const;

    /**
     * Return a copy of the present object, which is newly allocated on the
     * heap. Ownership of that object is transferred to the caller of this
     * function.
     */
    virtual std::unique_ptr<PatternBase> clone () const;

    /**
     * Creates new object if the start of description matches
     * description_init.  Ownership of that object is transferred to the
     * caller of this function.
     */
    static std::unique_ptr<Map> create (const std::string &description);

    /**
     * Determine an estimate for the memory consumption (in bytes) of this
     * object.
     */
    std::size_t memory_consumption () const;

    /**
     * Return a reference to the key pattern.
     */
    const PatternBase &get_key_pattern() const;

    /**
     * Return a reference to the value pattern.
     */
    const PatternBase &get_value_pattern() const;

    /**
     * Return the separator of the map entries.
     */
    const std::string &get_separator() const;

    /**
     * Return the key-value separator.
     */
    const std::string &get_key_value_separator() const;

    /**
     * @addtogroup Exceptions
     * @{
     */

    /**
     * Exception.
     */
    DeclException2 (ExcInvalidRange,
                    int, int,
                    << "The values " << arg1 << " and " << arg2
                    << " do not form a valid range.");
    //@}
  private:
    /**
     * Copy of the patterns that each key and each value of the map has to
     * satisfy.
     */
    std::unique_ptr<PatternBase> key_pattern;
    std::unique_ptr<PatternBase> value_pattern;

    /**
     * Minimum number of elements the list must have.
     */
    const unsigned int min_elements;

    /**
     * Maximum number of elements the list must have.
     */
    const unsigned int max_elements;

    /**
     * Separator between elements of the list.
     */
    const std::string separator;


    /**
     * Separator between keys and values.
     */
    const std::string key_value_separator;

    /**
     * Initial part of description
     */
    static const char *description_init;
  };


  /**
   * This class is much like the Selection class, but it allows the input to
   * be a comma-separated list of values which each have to be given in the
   * constructor argument. The input is allowed to be empty or contain values
   * more than once and have an arbitrary number of spaces around commas. Of
   * course commas are not allowed inside the values given to the constructor.
   *
   * For example, if the string to the constructor was <tt>"ucd|gmv|eps"</tt>,
   * then the following would be legal inputs: "eps", "gmv, eps", or "".
   */
  class MultipleSelection : public PatternBase
  {
  public:
    /**
     * Constructor. @p seq is a list of valid options separated by "|".
     */
    MultipleSelection (const std::string &seq);

    /**
     * Return <tt>true</tt> if the string is an element of the description
     * list passed to the constructor.
     */
    virtual bool match (const std::string &test_string) const;

    /**
     * Return a description of the pattern that valid strings are expected to
     * match. Here, this is the list of valid strings passed to the
     * constructor.
     */
    virtual std::string description (const OutputStyle style=Machine) const;

    /**
     * Return a copy of the present object, which is newly allocated on the
     * heap. Ownership of that object is transferred to the caller of this
     * function.
     */
    virtual std::unique_ptr<PatternBase> clone () const;

    /**
     * Creates new object if the start of description matches
     * description_init.  Ownership of that object is transferred to the
     * caller of this function.
     */
    static std::unique_ptr<MultipleSelection> create (const std::string &description);

    /**
     * Determine an estimate for the memory consumption (in bytes) of this
     * object.
     */
    std::size_t memory_consumption () const;

    /**
     * @addtogroup Exceptions
     * @{
     */

    /**
     * Exception.
     */
    DeclException1 (ExcCommasNotAllowed,
                    int,
                    << "A comma was found at position " << arg1
                    << " of your input string, but commas are not allowed here.");
    //@}
  private:
    /**
     * List of valid strings as passed to the constructor. We don't make this
     * string constant, as we process it somewhat in the constructor.
     */
    std::string sequence;

    /**
     * Initial part of description
     */
    static const char *description_init;
  };

  /**
   * Test for the string being either "true" or "false". This is mapped to the
   * Selection class.
   */
  class Bool : public Selection
  {
  public:
    /**
     * Constructor.
     */
    Bool ();

    /**
     * Return a description of the pattern that valid strings are expected to
     * match.
     */
    virtual std::string description (const OutputStyle style=Machine) const;

    /**
     * Return a copy of the present object, which is newly allocated on the
     * heap. Ownership of that object is transferred to the caller of this
     * function.
     */
    virtual std::unique_ptr<PatternBase> clone () const;

    /**
     * Creates new object if the start of description matches
     * description_init.  Ownership of that object is transferred to the
     * caller of this function.
     */
    static std::unique_ptr<Bool> create(const std::string &description);

  private:
    /**
     * Initial part of description
     */
    static const char *description_init;
  };

  /**
   * Always returns <tt>true</tt> when testing a string.
   */
  class Anything : public PatternBase
  {
  public:
    /**
     * Constructor. (Allow for at least one non-virtual function in this
     * class, as otherwise sometimes no virtual table is emitted.)
     */
    Anything ();

    /**
     * Return <tt>true</tt> if the string matches its constraints, i.e.
     * always.
     */
    virtual bool match (const std::string &test_string) const;

    /**
     * Return a description of the pattern that valid strings are expected to
     * match. Here, this is the string <tt>"[Anything]"</tt>.
     */
    virtual std::string description (const OutputStyle style=Machine) const;

    /**
     * Return a copy of the present object, which is newly allocated on the
     * heap. Ownership of that object is transferred to the caller of this
     * function.
     */
    virtual std::unique_ptr<PatternBase> clone () const;

    /**
     * Creates new object if the start of description matches
     * description_init.  Ownership of that object is transferred to the
     * caller of this function.
     */
    static std::unique_ptr<Anything> create(const std::string &description);

  private:
    /**
     * Initial part of description
     */
    static const char *description_init;
  };


  /**
   * A pattern that can be used to indicate when a parameter is intended to be
   * the name of a file. By itself, this class does not check whether the
   * string that is given in a parameter file actually corresponds to an
   * existing file (it could, for example, be the name of a file to which you
   * want to write output). Functionally, the class is therefore equivalent to
   * the Anything class. However, it allows to specify the <i>intent</i> of a
   * parameter. The flag given to the constructor also allows to specify
   * whether the file is supposed to be an input or output file.
   *
   * The reason for the existence of this class is to support graphical user
   * interfaces for editing parameter files. These may open a file selection
   * dialog if the filename is supposed to represent an input file.
   */
  class FileName : public PatternBase
  {
  public:
    /**
     * Files can be used for input or output. This can be specified in the
     * constructor by choosing the flag <tt>type</tt>.
     */
    enum FileType
    {
      /**
       * Open for input.
       */
      input = 0,
      /**
       * Open for output.
       */
      output = 1
    };

    /**
     * Constructor.  The type of the file can be specified by choosing the
     * flag.
     */
    FileName (const FileType type = input);

    /**
     * Return <tt>true</tt> if the string matches its constraints, i.e.
     * always.
     */
    virtual bool match (const std::string &test_string) const;

    /**
     * Return a description of the pattern that valid strings are expected to
     * match. Here, this is the string <tt>"[Filename]"</tt>.
     */
    virtual std::string description (const OutputStyle style=Machine) const;

    /**
     * Return a copy of the present object, which is newly allocated on the
     * heap. Ownership of that object is transferred to the caller of this
     * function.
     */
    virtual std::unique_ptr<PatternBase> clone () const;

    /**
     * file type flag
     */
    FileType  file_type;

    /**
     * Creates new object if the start of description matches
     * description_init.  Ownership of that object is transferred to the
     * caller of this function.
     */
    static std::unique_ptr<FileName> create (const std::string &description);

  private:
    /**
     * Initial part of description
     */
    static const char *description_init;
  };


  /**
   * A pattern that can be used to indicate when a parameter is intended to be
   * the name of a directory. By itself, this class does not check whether the
   * string that is given in a parameter file actually corresponds to an
   * existing directory. Functionally, the class is therefore equivalent to
   * the Anything class. However, it allows to specify the <i>intent</i> of a
   * parameter.
   *
   * The reason for the existence of this class is to support graphical user
   * interfaces for editing parameter files. These may open a file selection
   * dialog to select or create a directory.
   */
  class DirectoryName : public PatternBase
  {
  public:
    /**
     * Constructor.
     */
    DirectoryName ();

    /**
     * Return <tt>true</tt> if the string matches its constraints, i.e.
     * always.
     */
    virtual bool match (const std::string &test_string) const;

    /**
     * Return a description of the pattern that valid strings are expected to
     * match. Here, this is the string <tt>"[Filename]"</tt>.
     */
    virtual std::string description (const OutputStyle style=Machine) const;

    /**
     * Return a copy of the present object, which is newly allocated on the
     * heap. Ownership of that object is transferred to the caller of this
     * function.
     */
    virtual std::unique_ptr<PatternBase> clone () const;

    /**
     * Creates new object if the start of description matches
     * description_init.  Ownership of that object is transferred to the
     * caller of this function.
     */
    static std::unique_ptr<DirectoryName> create(const std::string &description);

  private:
    /**
     * Initial part of description
     */
    static const char *description_init;
  };


  /**
   * Namespace for a few classes and functions that act on values and patterns,
   * and allow to convert from non elementary types to strings and vice versa.
   *
   * A typical usage of these tools is in the following example:
   *
   * @code
   * typedef std::vector<unsigned int> T;
   *
   * T vec(3);
   * vec[0] = 1;
   * vec[1] = 3;
   * vec[2] = 5;
   *
   * auto pattern = Patterns::Tools::Convert<T>::to_pattern();
   *
   * std::cout << pattern->description() << std::endl;
   * // [List of <[Integer]> of length 0...4294967295 (inclusive)]
   *
   * auto s = Patterns::Tools::Convert<T>::to_string(vec);
   * std::cout << s << std::endl;
   * // 1, 2, 3
   *
   * auto vec = Patterns::Tools::Convert<T>::to_value("2,3,4,5");
   * // now vec has size 4, and contains the elements 2,3,4,5
   *
   * std::cout << internal::RankInfo<T>::list_rank << std::endl; // Outputs 1
   * std::cout << internal::RankInfo<T>::map_rank  << std::endl; // Outputs 0
   * @endcode
   *
   * Convert<T> is used by the function Patterns::Tools::add_parameter() in this
   * namespace. Internally it uses the internal::RankInfo<T> class to decide how
   * many different separators are required to convert the given type to a string.
   *
   * For example, to write vectors of vectors, the default is to use "," for the
   * first (inner) separator, and ";" for the second (outer) separator, i.e.
   *
   * @code
   * std::vector<std::vector<unsigned int>> vec;
   * vec = Convert<decltype(vec)>::to_value("1,2,3 ; 4,5,6");
   *
   * s = convert<decltype(vec[0])>::to_string(vec[0]);
   * // s now contains the string "1,2,3"
   * @endcode
   *
   * Separators for Patterns::List and Patterns::Map compatible types are
   * selected according to the
   * rank of the list and map objects, using the arrays
   * Patterns::Tools::internal::default_list_separator and
   * Patterns::Tools::internal::default_map_separator.
   *
   * They are currently set to:
   *
   * @code
   * default_list_separator{{","  ,  ";"  ,  "|"  ,   "%"}};
   * default_map_separator {{":"  ,  "="  ,  "@"  ,   "#"}};
   * @endcode
   *
   * When one needs a mixture of Patterns::List and Patterns::Map types, their
   * RankInfo is computed by taking the maximum of the vector_rank of the Key and
   * of the Value type, so that, for example, it is possible to have the following
   * @code
   * ... // Build compare class
   * std::map<std::vector<unsigned int>, std::vector<double>, compare> map;
   *
   * map = convert<decltype(map)>::to_value("1,2,3 : 5.0,6.0,7.0  ; 8,9,10 :
   * 11.0,12.0,13.0");
   *
   * @endcode
   *
   * Some non elementary types are supported, like Point(), or
   * std::complex<double>. If you wish to support more types, you have to
   * specialize the Convert struct as well as the RankInfo struct.
   *
   * @ingroup input
   * @author Luca Heltai, 2017
   */
  namespace Tools
  {
    /**
    * Converter class. This class is used to generate strings and Patterns
    * associated to the given type, and to convert from a string to the given
    * type and viceversa.
    *
    * The second template parameter is used internally to allow for advanced
    * SFINAE (substitution failure is not an error) tricks used to specialise
    * this class for arbitrary STL containers and maps.
    *
    * @author Luca Heltai, 2017
    */
    template <class T, class Enable = void>
    struct Convert
    {

      /**
       * Return a std::unique_ptr to a Pattern that can be used to interpret a
       * string as the type of the template argument, and the other way around.
       *
       * While the current function (in the general Convert template) is deleted,
       * it is implemented and available in the specializations of the Convert
       * class template for particular kinds of template arguments @p T.
       */
      static std::unique_ptr<Patterns::PatternBase> to_pattern() = delete;

      /**
       * Return a string containing a textual version of the variable s. Use the
       * pattern passed to perform the conversion, or create and use a default
       * one.
       *
       * While the current function (in the general Convert template) is deleted,
       * it is implemented and available in the specializations of the Convert
       * class template for particular kinds of template arguments @p T.
       */
      static std::string to_string(const T &s,
                                   const std::unique_ptr<Patterns::PatternBase>
                                   &p = Convert<T>::to_pattern()) = delete;

      /**
       * Convert a string to a value, using the given pattern. Use the pattern
       * passed to perform the conversion, or create and use a default one.
       *
       * While the current function (in the general Convert template) is deleted,
       * it is implemented and available in the specializations of the Convert
       * class template for particular kinds of template arguments @p T.
       */
      static T to_value(const std::string &s,
                        const std::unique_ptr<Patterns::PatternBase> &p =
                          Convert<T>::to_pattern()) = delete;
    };



    /**
     * @addtogroup Exceptions
     * @{
     */

    /**
     * Exception.
     */
    DeclException2(ExcNoMatch,
                   std::string,
                   Patterns::PatternBase &,
                   << "The string " << arg1 << " does not match the pattern \""
                   << arg2.description()
                   << "\"");
    //@}
  }
}


// ---------------------- inline and template functions --------------------
namespace Patterns
{
  namespace Tools
  {
    namespace internal
    {
      /**
       * Store information about the rank types of the given class.
       *
       * A class has Rank equal to the number of different separators
       * that are required to uniquely identify its element(s) in a string.
       *
       * This class is used to detect wether the class T is compatible
       * with a Patterns::List pattern or with a Patterns::Map pattern.
       *
       * Objects like Point() or std::complex<double> are vector-likes, and
       * have vector_rank 1. Elementary types, like `int`, `unsigned int`,
       * `double`, etc. have vector_rank 0. `std::vector`, `std::list` and in
       * general containers have rank equal to 1 + vector_rank of the contained
       * type. Similarly for map types.
       *
       * A class with list_rank::value = 0 is either elementary or a
       * map. A class with map_rank::value = 0 is either a List compatible
       * class, or an elementary type.
       *
       * Elementary types are not compatible with Patterns::List, but non
       * elementary types, like Point(), or std::complex<double>, are compatible
       * with the List type. Adding more compatible types is a matter of adding a
       * specialization of this struct for the given type.
       *
       * @author Luca Heltai, 2017
       */
      template <class T, class Enable = void>
      struct RankInfo
      {
        static constexpr int list_rank = 0;
        static constexpr int map_rank = 0;
      };
    }

    // Arithmetic types
    template <class T>
    struct Convert<T, typename std::enable_if<std::is_arithmetic<T>::value>::type>
    {

      static std::unique_ptr<Patterns::PatternBase> to_pattern()
      {
        if (std::is_same<T,bool>::value)
          return std_cxx14::make_unique<Patterns::Bool>();
        else if (std::is_integral<T>::value)
          return std_cxx14::make_unique<Patterns::Integer>(
                   std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
        else if (std::is_floating_point<T>::value)
          return std_cxx14::make_unique<Patterns::Double>(
                   -std::numeric_limits<T>::max(), std::numeric_limits<T>::max());
      }

      static std::string to_string(const T &value,
                                   const std::unique_ptr<Patterns::PatternBase>
                                   &p = Convert<T>::to_pattern())
      {
        std::string str;
        if (std::is_same<T, unsigned char>() || std::is_same<T, char>())
          str = std::to_string((int)value);
        else  if (std::is_same<T,bool>::value)
          str = value ? "true" : "false";
        else
          str = std::to_string(value);
        AssertThrow(p->match(str), ExcNoMatch(str, *p));
        return str;
      }

      static T to_value(const std::string &s,
                        const std::unique_ptr<Patterns::PatternBase> &p =
                          Convert<T>::to_pattern())
      {
        AssertThrow(p->match(s), ExcNoMatch(s, *p));
        T value;
        if (std::is_same<T,bool>::value)
          value = (s == "true");
        else
          {
            std::istringstream is(s);
            if (std::is_same<T, unsigned char>::value || std::is_same<T, char>::value)
              {
                int i;
                is >> i;
                value = i;
              }
            else
              is >> value;

            // If someone passes "123 abc" to the function, the method yelds an
            // integer 123 alright, but the space terminates the read from the string
            // although there is more to come. This case, however, is checked for in
            // the call p->match(s) at the beginning of this function, and would
            // throw earlier. Here it is safe to assume that if we didn't fail the
            // conversion with the operator >>, then we are good to go.
            AssertThrow(!is.fail(),
                        ExcMessage("Failed to convert from \"" + s +
                                   "\" to the type \"" +
                                   boost::core::demangle(typeid(T).name()) + "\""));
          }
        return value;
      }
    };

    namespace internal
    {
      const std::array<std::string, 4> default_list_separator {{","  ,  ";"  ,  "|"  ,   "%"}};
      const std::array<std::string, 4> default_map_separator {{":"  ,  "="  ,  "@"  ,   "#"}};

      //specialize a type for all of the STL containers and maps
      template <typename T>       struct is_list_compatible : std::false_type {};
      template <typename T, std::size_t N> struct is_list_compatible<std::array    <T,N>>     : std::true_type {};
      template <typename... Args> struct is_list_compatible<std::vector            <Args...>> : std::true_type {};
      template <typename... Args> struct is_list_compatible<std::deque             <Args...>> : std::true_type {};
      template <typename... Args> struct is_list_compatible<std::list              <Args...>> : std::true_type {};
      template <typename... Args> struct is_list_compatible<std::set               <Args...>> : std::true_type {};
      template <typename... Args> struct is_list_compatible<std::multiset          <Args...>> : std::true_type {};
      template <typename... Args> struct is_list_compatible<std::unordered_set     <Args...>> : std::true_type {};
      template <typename... Args> struct is_list_compatible<std::unordered_multiset<Args...>> : std::true_type {};

      template <typename T>       struct is_map_compatible : std::false_type {};
      template <typename... Args> struct is_map_compatible<std::map               <Args...>> : std::true_type {};
      template <typename... Args> struct is_map_compatible<std::multimap          <Args...>> : std::true_type {};
      template <typename... Args> struct is_map_compatible<std::unordered_map     <Args...>> : std::true_type {};
      template <typename... Args> struct is_map_compatible<std::unordered_multimap<Args...>> : std::true_type {};
    }

    // type trait to use the implementation type traits as well as decay the type
    template <typename T>
    struct is_list_compatible
    {
      static constexpr bool const value =
        internal::is_list_compatible<typename std::decay<T>::type>::value;
    };

    template <typename T>
    struct is_map_compatible
    {
      static constexpr bool const value =
        internal::is_map_compatible<typename std::decay<T>::type>::value;
    };

    namespace internal
    {
      // Rank of vector types
      template <class T>
      struct RankInfo<T,
        typename std::enable_if<is_list_compatible<T>::value>::type>
      {
        static constexpr int list_rank =
          RankInfo<typename T::value_type>::list_rank + 1;
        static constexpr int map_rank =
          RankInfo<typename T::value_type>::map_rank;
      };

      // Rank of map types
      template <class T>
      struct RankInfo<T, typename std::enable_if<is_map_compatible<T>::value>::type>
      {
        static constexpr int list_rank =
          std_cxx14::max(internal::RankInfo<typename T::key_type>::list_rank,
                         RankInfo<typename T::mapped_type>::list_rank) +
          1;
        static constexpr int map_rank =
          std_cxx14::max(internal::RankInfo<typename T::key_type>::map_rank,
                         RankInfo<typename T::mapped_type>::map_rank) +
          1;
      };

      // Rank of Tensor types
      template <int rank, int dim, class Number>
      struct RankInfo<Tensor<rank, dim, Number>>
      {
        static constexpr int list_rank = rank + RankInfo<Number>::list_rank;
        static constexpr int map_rank = RankInfo<Number>::map_rank;
      };

      template <int dim, class Number>
      struct RankInfo<Point<dim, Number>> : RankInfo<Tensor<1, dim, Number>>
      {
      };

      // Rank of complex types
      template <class Number>
      struct RankInfo<std::complex<Number>>
      {
        static constexpr int list_rank = RankInfo<Number>::list_rank + 1;
        static constexpr int map_rank = RankInfo<Number>::map_rank;
      };

      template <class Key, class Value>
      struct RankInfo<std::pair<Key,Value>>
      {
        static constexpr int list_rank = std::max(RankInfo<Key>::list_rank, RankInfo<Value>::list_rank);
        static constexpr int map_rank = std::max(RankInfo<Key>::map_rank, RankInfo<Value>::map_rank)+1;
      };
    }

    // stl containers
    template <class T>
    struct Convert<T, typename std::enable_if<is_list_compatible<T>::value>::type>
    {
      static std::unique_ptr<Patterns::PatternBase> to_pattern()
      {
        static_assert(internal::RankInfo<T>::list_rank > 0,
                      "Cannot use this class for non List-compatible types.");
        return std_cxx14::make_unique<Patterns::List>(
                 *Convert<typename T::value_type>::to_pattern(),
                 0,
                 std::numeric_limits<unsigned int>::max(),
                 internal::default_list_separator[internal::RankInfo<T>::list_rank - 1]);
      }

      static std::string to_string(const T &t,
                                   const std::unique_ptr<Patterns::PatternBase>
                                   &pattern = Convert<T>::to_pattern())
      {
        auto p = dynamic_cast<const Patterns::List *>(pattern.get());
        AssertThrow(p, ExcMessage("I need a List pattern to convert a "
                                  "string to a List type."));
        auto base_p = p->get_base_pattern().clone();
        std::vector<std::string> vec(t.size());

        unsigned int i = 0;
        for (const auto &ti : t)
          vec[i++] = Convert<typename T::value_type>::to_string(ti, base_p);

        std::string s;
        if (vec.size() > 0)
          s = vec[0];
        for (unsigned int i = 1; i < vec.size(); ++i)
          s += p->get_separator() + " " + vec[i];

        AssertThrow(pattern->match(s), ExcNoMatch(s, *p));
        return s;
      }

      static T to_value(const std::string &s,
                        const std::unique_ptr<Patterns::PatternBase> &pattern =
                          Convert<T>::to_pattern())
      {

        AssertThrow(pattern->match(s), ExcNoMatch(s, *pattern));

        auto p = dynamic_cast<const Patterns::List *>(pattern.get());
        AssertThrow(p,ExcMessage("I need a List pattern to convert a string "
                                 "to a List type."));

        auto base_p = p->get_base_pattern().clone();
        T t;

        auto v = Utilities::split_string_list(s, p->get_separator());
        for (const auto &str : v)
          t.insert(t.end(),
                   Convert<typename T::value_type>::to_value(str, base_p));

        return t;
      }
    };

    // stl maps
    template <class T>
    struct Convert<T, typename std::enable_if<is_map_compatible<T>::value>::type>
    {
      static std::unique_ptr<Patterns::PatternBase> to_pattern()
      {
        static_assert(internal::RankInfo<T>::list_rank > 0,
                      "Cannot use this class for non List-compatible types.");
        static_assert(internal::RankInfo<T>::map_rank > 0,
                      "Cannot use this class for non Map-compatible types.");
        return std_cxx14::make_unique<Patterns::Map>(
                 *Convert<typename T::key_type>::to_pattern(),
                 *Convert<typename T::mapped_type>::to_pattern(),
                 0,
                 std::numeric_limits<unsigned int>::max(),
                 internal::default_list_separator[internal::RankInfo<T>::list_rank - 1],
                 internal::default_map_separator[internal::RankInfo<T>::map_rank - 1]);
      }

      static std::string to_string(const T &t,
                                   const std::unique_ptr<Patterns::PatternBase>
                                   &pattern = Convert<T>::to_pattern())
      {
        auto p = dynamic_cast<const Patterns::Map *>(pattern.get());
        AssertThrow(p, ExcMessage("I need a Map pattern to convert a string to "
                                  "a Map compatbile type."));
        auto key_p = p->get_key_pattern().clone();
        auto val_p = p->get_value_pattern().clone();
        std::vector<std::string> vec(t.size());

        unsigned int i = 0;
        for (const auto &ti : t)
          vec[i++] =
            Convert<typename T::key_type>::to_string(ti.first, key_p) +
            p->get_key_value_separator() +
            Convert<typename T::mapped_type>::to_string(ti.second, val_p);

        std::string s;
        if (vec.size() > 0)
          s = vec[0];
        for (unsigned int i = 1; i < vec.size(); ++i)
          s += p->get_separator() + " " + vec[i];

        AssertThrow(p->match(s), ExcNoMatch(s, *p));
        return s;
      }

      static T to_value(const std::string &s,
                        const std::unique_ptr<Patterns::PatternBase> &pattern =
                          Convert<T>::to_pattern())
      {

        AssertThrow(pattern->match(s), ExcNoMatch(s, *pattern));

        auto p = dynamic_cast<const Patterns::Map *>(pattern.get());
        AssertThrow(p, ExcMessage("I need a Map pattern to convert a "
                                  "string to a Map compatible type."));

        auto key_p = p->get_key_pattern().clone();
        auto val_p = p->get_value_pattern().clone();
        T t;

        auto v = Utilities::split_string_list(s, p->get_separator());
        for (const auto &str : v)
          {
            auto key_val =
              Utilities::split_string_list(str, p->get_key_value_separator());
            AssertDimension(key_val.size(), 2);
            t.insert(std::make_pair(
                       Convert<typename T::key_type>::to_value(key_val[0], key_p),
                       Convert<typename T::mapped_type>::to_value(key_val[1])));
          }

        return t;
      }
    };

    // Tensors
    template <int rank, int dim, class Number>
    struct Convert<Tensor<rank, dim, Number>>
    {
      typedef Tensor<rank, dim, Number> T;
      static std::unique_ptr<Patterns::PatternBase> to_pattern()
      {
        static_assert(internal::RankInfo<T>::list_rank > 0,
                      "Cannot use this class for non List-compatible types.");
        return std_cxx14::make_unique<Patterns::List>(
                 *Convert<typename T::value_type>::to_pattern(),
                 dim,
                 dim,
                 internal::default_list_separator[internal::RankInfo<T>::list_rank - 1]);
      }

      static std::string to_string(const T &t,
                                   const std::unique_ptr<Patterns::PatternBase>
                                   &pattern = Convert<T>::to_pattern())
      {

        auto p = dynamic_cast<const Patterns::List *>(pattern.get());
        AssertThrow(p,ExcMessage("I need a List pattern to convert a string "
                                 "to a List compatbile type."));
        auto base_p = p->get_base_pattern().clone();
        std::vector<std::string> vec(dim);

        for (unsigned int i = 0; i < dim; ++i)
          vec[i] = Convert<typename T::value_type>::to_string(t[i], base_p);

        std::string s;
        if (vec.size() > 0)
          s = vec[0];
        for (unsigned int i = 1; i < vec.size(); ++i)
          s += p->get_separator() + " " + vec[i];

        AssertThrow(p->match(s), ExcNoMatch(s, *p));
        return s;
      }

      static T to_value(const std::string &s,
                        const std::unique_ptr<Patterns::PatternBase> &pattern =
                          Convert<T>::to_pattern())
      {

        AssertThrow(pattern->match(s), ExcNoMatch(s, *pattern));

        auto p = dynamic_cast<const Patterns::List *>(pattern.get());
        AssertThrow(p,ExcMessage("I need a List pattern to convert a string "
                                 "to a List compatbile type."));

        auto base_p = p->get_base_pattern().clone();
        T t;

        auto v = Utilities::split_string_list(s, p->get_separator());
        unsigned int i = 0;
        for (const auto &str : v)
          t[i++] = Convert<typename T::value_type>::to_value(str, base_p);

        return t;
      }
    };

    // Points
    template <int dim, class Number>
    struct Convert<Point<dim, Number>>
    {

      typedef Point<dim, Number> T;

      static std::unique_ptr<Patterns::PatternBase> to_pattern()
      {
        return Convert<Tensor<1, dim, Number>>::to_pattern();
      }

      static std::string to_string(const T &t,
                                   const std::unique_ptr<Patterns::PatternBase>
                                   &pattern = Convert<T>::to_pattern())
      {
        return Convert<Tensor<1, dim, Number>>::to_string(
                 Tensor<1, dim, Number>(t), pattern);
      }

      static T to_value(const std::string &s,
                        const std::unique_ptr<Patterns::PatternBase> &pattern =
                          Convert<T>::to_pattern())
      {
        return T(Convert<Tensor<1, dim, Number>>::to_value(s, pattern));
      }
    };

    // Complex numbers
    template <class Number>
    struct Convert<std::complex<Number>>
    {
      typedef std::complex<Number> T;

      static std::unique_ptr<Patterns::PatternBase> to_pattern()
      {
        static_assert(internal::RankInfo<T>::list_rank > 0,
                      "Cannot use this class for non List-compatible types.");
        return std_cxx14::make_unique<Patterns::List>(
                 *Convert<typename T::value_type>::to_pattern(),
                 2,
                 2,
                 internal::default_list_separator[internal::RankInfo<T>::list_rank - 1]);
      }

      static std::string to_string(const T &t,
                                   const std::unique_ptr<Patterns::PatternBase>
                                   &pattern = Convert<T>::to_pattern())
      {

        auto p = dynamic_cast<const Patterns::List *>(pattern.get());
        AssertThrow(p,ExcMessage("I need a List pattern to convert a string "
                                 "to a List compatbile type."));

        auto base_p = p->get_base_pattern().clone();
        std::string s =
          Convert<typename T::value_type>::to_string(t.real(), base_p) +
          p->get_separator() + " " +
          Convert<typename T::value_type>::to_string(t.imag(), base_p);

        AssertThrow(pattern->match(s), ExcNoMatch(s, *p));
        return s;
      }

      /**
       * Convert a string to a value, using the given pattern, or a default one.
       */
      static T to_value(const std::string &s,
                        const std::unique_ptr<Patterns::PatternBase> &pattern =
                          Convert<T>::to_pattern())
      {

        AssertThrow(pattern->match(s), ExcNoMatch(s, *pattern));

        auto p = dynamic_cast<const Patterns::List *>(pattern.get());
        AssertThrow(p,ExcMessage("I need a List pattern to convert a string "
                                 "to a List compatbile type."));

        auto base_p = p->get_base_pattern().clone();

        auto v = Utilities::split_string_list(s, p->get_separator());
        AssertDimension(v.size(), 2);
        T t(Convert<typename T::value_type>::to_value(v[0], base_p),
            Convert<typename T::value_type>::to_value(v[1], base_p));
        return t;
      }
    };

    // Strings
    template <>
    struct Convert<std::string>
    {
      typedef std::string T;

      static std::unique_ptr<Patterns::PatternBase> to_pattern()
      {
        return std_cxx14::make_unique<Patterns::Anything>();
      }

      static std::string to_string(const T &t,
                                   const std::unique_ptr<Patterns::PatternBase>
                                   &pattern = Convert<T>::to_pattern())
      {
        AssertThrow(pattern->match(t), ExcNoMatch(t, *pattern));
        return t;
      }

      static T to_value(const std::string &s,
                        const std::unique_ptr<Patterns::PatternBase> &pattern =
                          Convert<T>::to_pattern())
      {
        AssertThrow(pattern->match(s), ExcNoMatch(s, *pattern));
        return s;
      }
    };


    // Pairs
    template <class Key, class Value>
    struct Convert<std::pair<Key,Value>>
    {
      typedef std::pair<Key,Value> T;

      static std::unique_ptr<Patterns::PatternBase> to_pattern()
      {
        static_assert(internal::RankInfo<T>::map_rank > 0,
                      "Cannot use this class for non Map-compatible types.");
        return std_cxx14::make_unique<Patterns::Map>(
                 *Convert<Key>::to_pattern(),
                 *Convert<Value>::to_pattern(),
                 1, 1,
                 // We keep the same list separator of the previous level, as this is
                 // a map with only 1 possible entry
                 internal::default_list_separator[internal::RankInfo<T>::list_rank],
                 internal::default_map_separator[internal::RankInfo<T>::map_rank - 1]);
      }

      static std::string to_string(const T &t,
                                   const std::unique_ptr<Patterns::PatternBase>
                                   &pattern = Convert<T>::to_pattern())
      {
        std::unordered_map<Key, Value> m;
        m.insert(t);
        std:: string s = Convert<decltype(m)>::to_string(m, pattern);
        AssertThrow(pattern->match(s), ExcNoMatch(s, *pattern));
        return s;
      }

      static T to_value(const std::string &s,
                        const std::unique_ptr<Patterns::PatternBase> &pattern =
                          Convert<T>::to_pattern())
      {
        std::unordered_map<Key, Value> m;
        m = Convert<decltype(m)>::to_value(s, pattern);
        return *m.begin();
      }
    };
  }
}


DEAL_II_NAMESPACE_CLOSE

#endif