#ifndef O2_MID_EfficiencySpec_H
#define O2_MID_EfficiencySpec_H

#include "Framework/DataProcessorSpec.h"

namespace o2
{
namespace mid
{

framework::DataProcessorSpec getEfficiencySpec(const char* name = "Efficiency");

} // namespace mid
} // namespace o2

#endif // O2_MID_EfficiencySpec_H_
