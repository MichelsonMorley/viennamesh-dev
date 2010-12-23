/*
 *  divide_by_inplace.hpp
 *  MTL4
 *
 *  Created by Hui Li (huil@Princeton.EDU)
 *
 */

#ifndef MTL_DIVIDE_BY_INPLACE_INCLUDE
#define MTL_DIVIDE_BY_INPLACE_INCLUDE

#include <boost/numeric/mtl/mtl_fwd.hpp>
#include <boost/numeric/mtl/utility/tag.hpp>
#include <boost/numeric/mtl/operation/assign_each_nonzero.hpp>

#include <boost/lambda/lambda.hpp>


namespace mtl {
	
	
	/// Divide collection \p c (from right) by scalar factor \p alpha; \p c is altered
	template <typename Factor, typename Collection>
	void divide_by_inplace(Collection& c, const Factor& alpha, tag::scalar)
	{
		assign_each_nonzero(c, boost::lambda::_1 / alpha);
	}
	
	/// Divide collection \p c (from right) by factor \p alpha; \p c is altered
	template <typename Factor, typename Collection>
	void divide_by_inplace(Collection& c, const Factor& alpha)
	{
		divide_by_inplace(c, alpha, typename traits::category<Factor>::type());
	}
	
	
} // namespace mtl

#endif // MTL_DIVIDE_BY_INPLACE_INCLUDE
