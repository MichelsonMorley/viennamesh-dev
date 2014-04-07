#ifndef VIENNAMESH_CORE_ALGORITHM_FACTORY_HPP
#define VIENNAMESH_CORE_ALGORITHM_FACTORY_HPP

#include "viennamesh/core/algorithm.hpp"


namespace viennamesh
{


  class base_algorithm_information : public enable_shared_from_this<base_algorithm_information>
  {
  public:

    virtual ~base_algorithm_information() {}

    virtual algorithm_handle create() const = 0;
    virtual string const & name() const = 0;
    virtual std::map<string, string> const & features() const = 0;
  };

  template<typename AlgorithmT>
  class algorithm_information : public base_algorithm_information
  {
  public:
    algorithm_information(string const & algorithm_name_) : algorithm_name(algorithm_name_) {}

    algorithm_handle create() const { return algorithm_handle( new AlgorithmT() ); }
    string const & name() const { return algorithm_name; }
    std::map<string, string> const & features() const { return feature_map(); }

    static std::map<string, string> & feature_map()
    {
      static std::map<string, string> feature_map_;
      return feature_map_;
    }

    string algorithm_name;
  };


  template<typename AlgorithmT>
  void set_feature( string const & feature_key, string const & feature_value )
  {
    algorithm_information<AlgorithmT>::feature_map()[feature_key] = feature_value;
  }


  typedef shared_ptr<base_algorithm_information> AlgorithmInformationHandle;


  class algorithm_factory_t
  {
  public:

    algorithm_factory_t();

    template<typename AlgorithmT>
    void register_algorithm(string const & algorithm_name)
    {
      std::map<string, AlgorithmInformationHandle>::iterator it = algorithms.find(algorithm_name);
      if (it != algorithms.end())
        return;

      algorithms.insert( std::make_pair(algorithm_name, AlgorithmInformationHandle( new algorithm_information<AlgorithmT>(algorithm_name))) );
    }

    std::list<AlgorithmInformationHandle> matching_algorithms( std::string const & expression ) const
    {
      std::list<AlgorithmInformationHandle> result;

      string feature_key = expression.substr(0, expression.find("=="));
      string feature_value = expression.substr(expression.find("==")+2);

      for (std::map<string, AlgorithmInformationHandle>::const_iterator ait = algorithms.begin(); ait != algorithms.end(); ++ait)
      {
        std::map<string, string> const & features = ait->second->features();
        std::map<string, string>::const_iterator fit = features.find(feature_key);
        if ((fit != features.end()) && (fit->second == feature_value))
          result.push_back(ait->second);
      }

      return result;
    }

    algorithm_handle create_from_name(std::string const & algorithm_name) const
    {
      std::map<string, AlgorithmInformationHandle>::const_iterator it = algorithms.find(algorithm_name);
      if (it == algorithms.end())
        return algorithm_handle();
      return it->second->create();
    }

    algorithm_handle create_from_expression(std::string const & expression) const
    {
      std::list<AlgorithmInformationHandle> algos = matching_algorithms(expression);
      if (algos.empty())
        return algorithm_handle();
      return algos.front()->create();
    }


  private:

    std::map<string, AlgorithmInformationHandle> algorithms;

  };





  algorithm_factory_t & algorithm_factory();

  template<typename AlgorithmT>
  class register_algorithm_handle
  {
  public:

    register_algorithm_handle(string const & algorithm_name)
    {
      algorithm_factory().register_algorithm<AlgorithmT>(algorithm_name);
    }

  private:
  };


}



#endif