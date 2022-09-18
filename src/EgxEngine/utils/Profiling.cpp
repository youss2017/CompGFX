#include "Profiling.hpp"

#if PROFILE_ENABLED == 2
#include <vector>
namespace Utils
{
    std::vector<ProfileResult> __PROFILE_RESULT_NAME;
}
#endif
