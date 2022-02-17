#include "ghua.h"

class Fenrir : public GHUA::Application {

};

GHUA::Application* GHUA::CreateApplication() {
	return new Fenrir();
}