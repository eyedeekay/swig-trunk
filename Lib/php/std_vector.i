/* -----------------------------------------------------------------------------
 * std_vector.i
 * ----------------------------------------------------------------------------- */

%include <std_common.i>

%{
#include <vector>
#include <stdexcept>
%}

namespace std {

  template<class T> class vector {
  public:
    typedef size_t size_type;
    typedef T value_type;
    typedef const value_type& const_reference;
    vector();
    vector(size_type n);
    size_type size() const;
    size_type capacity() const;
    void reserve(size_type n);
    void clear();
    %rename(push) push_back;
    void push_back(const value_type& x);
    %extend {
      bool is_empty() const {
        return $self->empty();
      }
      T pop() throw (std::out_of_range) {
        if (self->size() == 0)
          throw std::out_of_range("pop from empty vector");
        T x = self->back();
        self->pop_back();
        return x;
      }
      const_reference get(int i) throw (std::out_of_range) {
        int size = int(self->size());
        if (i>=0 && i<size)
          return (*self)[i];
        else
          throw std::out_of_range("vector index out of range");
      }
      void set(int i, const value_type& val) throw (std::out_of_range) {
        int size = int(self->size());
        if (i>=0 && i<size)
          (*self)[i] = val;
        else
          throw std::out_of_range("vector index out of range");
      }
    }
  };

  // bool specialization
  template<> class vector<bool> {
  public:
    typedef size_t size_type;
    typedef bool value_type;
    typedef bool const_reference;
    vector();
    vector(size_type n);
    size_type size() const;
    size_type capacity() const;
    void reserve(size_type n);
    void clear();
    %rename(push) push_back;
    void push_back(const value_type& x);
    %extend {
      bool is_empty() const {
        return $self->empty();
      }
      bool pop() throw (std::out_of_range) {
        if (self->size() == 0)
          throw std::out_of_range("pop from empty vector");
        bool x = self->back();
        self->pop_back();
        return x;
      }
      const_reference get(int i) throw (std::out_of_range) {
        int size = int(self->size());
        if (i>=0 && i<size)
          return (*self)[i];
        else
          throw std::out_of_range("vector index out of range");
      }
      void set(int i, const value_type& val) throw (std::out_of_range) {
        int size = int(self->size());
        if (i>=0 && i<size)
          (*self)[i] = val;
        else
          throw std::out_of_range("vector index out of range");
      }
    }
  };
}

%define specialize_std_vector(T)
#warning "specialize_std_vector - specialization for type T no longer needed"
%enddef


