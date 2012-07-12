#include "common.h"
#include "libgod.h"
#include "container.h"
#include "godpoint.h"
#include "godset.h"
#include "godunion.h"
#include "storage.h"
#include "goderror.h"
#include "asserts.h"

namespace asn1
{
#include "God.h"
}
#include "godasn1.h"
#include "godbridge.h"

namespace libgod
{

	// does not work with pointers
	template <class T>
	void zeroed(T& t, 
					typename boost::disable_if<boost::is_pointer<T> >::type* dummy = 0)
	{
		memset(&t, 0, sizeof(T));
	}

	template <class T>
	T* newzeroed()
	{
		T* ptr = new T;
		zeroed(*ptr);
		return ptr;
	}
	
	void GodBridge::convertToASN1Data (const Point& apoint, asn1::Point* asnPoint)
	{
		const std::string errReason = "Converting point to ASN1 failed";
		asn1::REAL_t *dataReal;

		zeroed(*asnPoint);

		// god.union.set.point.parameters
		for (size_t ind = 0; ind < apoint.dimParameter(); ++ind)
		{
			dataReal = newzeroed<asn1::REAL_t>();
			if (asn1::asn_double2REAL(dataReal, apoint.parameterAt(ind)))
				throw GodError(errReason, errno);
			if (ASN_SEQUENCE_ADD(&asnPoint->parameters, dataReal))
				throw GodError(errReason, errno);
		}
		// god.union.set.point.criteria
		for (size_t ind = 0; ind < apoint.dimCriteria(); ++ind)
		{
			dataReal = newzeroed<asn1::REAL_t>();
			if (asn1::asn_double2REAL(dataReal, apoint.criterionAt(ind)))
				throw GodError(errReason, errno);
			if (ASN_SEQUENCE_ADD(&asnPoint->criteria, dataReal))
				throw GodError(errReason, errno);
		}
	}
	
	void GodBridge::convertToASN1Data (const Set& aset, asn1::Set* asnSet)
	{
		const std::string errReason = "Converting a set to ASN1 failed";
		asn1::Point *asnPoint;

		zeroed(*asnSet);

		// god.union.set
		if (asn1::asn_long2INTEGER(&asnSet->pointCount, aset.size()))
			throw GodError(errReason, errno);

		for (Set::const_iterator itPoint = aset.begin(); 
				itPoint != aset.end(); ++itPoint)
		{
			asnPoint = newzeroed<asn1::Point>();
			convertToASN1Data(*itPoint, asnPoint);

			// push the point to the set
			if (ASN_SEQUENCE_ADD(&asnSet->points, asnPoint))
				throw GodError(errReason, errno);
		}
	}

	void GodBridge::convertToASN1Data (const Union& aunion, asn1::God* asnGod)
	{
		const int formatVersion = 1;
		const std::string errReason = "Converting to ASN1 failed";
		asn1::Set *asnSet;

		zeroed(*asnGod);

		// god.header
		if (asn1::asn_long2INTEGER(&asnGod->header.version, formatVersion))
			throw GodError(errReason, errno);
		
		// it's an utf8 string
		if (OCTET_STRING_fromString(&asnGod->header.comment, ""))
			throw GodError(errReason, errno);

		// god.metric
		if (asn1::asn_long2INTEGER(&asnGod->metric.parameterDim, aunion.dimParameter()))
			throw GodError(errReason, errno);
		if (asn1::asn_long2INTEGER(&asnGod->metric.criteriaDim, aunion.dimCriteria()))
			throw GodError(errReason, errno);
		if (asn1::asn_long2INTEGER(&asnGod->metric.setsCount, aunion.size()))
			throw GodError(errReason, errno);

		// god.union
		for (Union::const_iterator itSet = aunion.begin(); 
					itSet != aunion.end(); ++itSet)
		{
			// god.union.set
			asnSet = newzeroed<asn1::Set>();
			convertToASN1Data(*itSet, asnSet);
			if (ASN_SEQUENCE_ADD(&asnGod->Union, asnSet))
				throw GodError(errReason, errno);
		}

		ASSERT_EQUAL(asnGod->Union.list.count,aunion.size());
	}

	int stringFromOctetString (const asn1::UTF8String_t *s, std::string& result)
	{
		if (s->buf)
			result.assign(s->buf, s->buf + s->size);
		else
			result.clear();
		return 0;
	}

	void GodBridge::convertFromASN1Data (Union& aunion, const asn1::God* god)
	{
		const int formatVersion = 1;
		const std::string errReason = "Converting from ASN1 failed";

		long actualVersion;
		std::string comment;
		long dimParameter, dimCriteria, setsCount, pointCount;
		double value;

		asn1::Set *dataSet;
		asn1::Point *dataPoint;
		asn1::REAL_t *dataReal;

		// god.header
		if (asn1::asn_INTEGER2long(&god->header.version, &actualVersion))
			throw GodError(errReason, errno);
		if (actualVersion > formatVersion)
			throw GodError("Cannot read this newer format");
		
		// it's an utf8 string
		if (stringFromOctetString(&god->header.comment, comment))
			throw GodError(errReason, errno);

		// god.metric
		if (asn1::asn_INTEGER2long(&god->metric.parameterDim, &dimParameter))
			throw GodError(errReason, errno);
		if (asn1::asn_INTEGER2long(&god->metric.criteriaDim, &dimCriteria))
			throw GodError(errReason, errno);
		if (asn1::asn_INTEGER2long(&god->metric.setsCount, &setsCount))
			throw GodError(errReason, errno);

		// create non-bare union with right dimensions
		aunion = libgod::Union(dimParameter, dimCriteria);
		
		ASSERT_EQUAL(god->Union.list.count, setsCount);

		for (size_t setIndex = 0; setIndex < setsCount; ++setIndex)
		{
			libgod::Set& aset = aunion.add();

			dataSet = god->Union.list.array[setIndex];
			if (asn1::asn_INTEGER2long(&dataSet->pointCount, &pointCount))
				throw GodError(errReason, errno);

			ASSERT_EQUAL(dataSet->points.list.count, pointCount);

			for (size_t pointIndex = 0; pointIndex < pointCount; ++pointIndex)
			{
				libgod::Point& apoint = aset.add();

				dataPoint = dataSet->points.list.array[pointIndex];
			
				ASSERT_EQUAL(dataPoint->parameters.list.count, dimParameter);
				ASSERT_EQUAL(dataPoint->criteria.list.count, dimCriteria);

				for (size_t ind = 0; ind < dimParameter; ++ind)
				{
					if (asn1::asn_REAL2double(dataPoint->parameters.list.array[ind], &value))
						throw GodError(errReason, errno);
					apoint.setParameterAt(ind, value);
				}
				for (size_t ind = 0; ind < dimCriteria; ++ind)
				{
					if (asn1::asn_REAL2double(dataPoint->criteria.list.array[ind], &value))
						throw GodError(errReason, errno);
					apoint.setCriterionAt(ind, value);
				}
			}
		}
	}


};
