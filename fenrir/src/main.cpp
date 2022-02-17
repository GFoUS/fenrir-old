#include "ghua.h"

class Fenrir : public GHUA::Application {
public:
	Fenrir() : GHUA::Application("Fenrir") {}
};

GHUA::Application* GHUA::CreateApplication() {
	return new Fenrir();
}