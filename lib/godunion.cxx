#include "common.h"
#include "libgod.h"
#include "container.h"
#include "godpoint.h"
#include "godset.h"
#include "godunion.h"
#include "godmetadata.h"
#include "comparable.h"

namespace libgod
{
	Union::Union()
		: Union::BaseType("union")
	{
	}

	Union::Union(size_t dimParameter, size_t dimCriteria)
		: Union::BaseType("union", dimParameter, dimCriteria)
	{
	}

	Union::Union (const Union& rhs)
		: Union::BaseType(rhs)
	{
	}
		
	bool Union::operator== (const Union& rhs) const
	{
		return 
				dimParameter() == rhs.dimParameter() &&
				dimCriteria() == rhs.dimCriteria() &&
				size() == rhs.size();
	}

	bool Union::operator!= (const Union& rhs) const
	{
		return !(*this == rhs);
	}
	
	bool Union::invokeEquals (const Union& rhs) const
	{
		return Union::BaseType::deepEquals(rhs);
	}

	bool Union::deepEquals (const Union& rhs) const
	{
		// fetch items from both unions to temporary unions
		Union utempLhs = *this;
		Union utempRhs = rhs;

		// sort both copies
		utempLhs.sort(Comparable());
		utempRhs.sort(Comparable());

		// call deepEquals on them
		return utempLhs.invokeEquals(utempRhs);
	}
	
};


