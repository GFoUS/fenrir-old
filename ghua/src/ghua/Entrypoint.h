#include "Application.h"

namespace GHUA
{
    extern GHUA::Application *CreateApplication();
}

int main()
{
    auto *app = GHUA::CreateApplication();
    app->Run();
    delete app;
    return 0;
}
