#include "Pass.hpp"

void Application::Pass::Launch(void* pArgs)
{
	PassPrepare* p = (PassPrepare*)pArgs;
	p->pOutCmd = p->self->Prepare(p->nFrameIndex, Global::Time, Global::TimeFromStart);
}
