#include "engine.h"

#define WIDTH 3840
#define HEIGHT 2160
#define FPS 60
const int DELAY_TIME = 1000.0f / FPS;

Engine* engine = 0;
int main(int argc, char* argv[])
{
	Uint32 frameStart, frameTime;
	engine = new Engine();
	engine->init("Engine", 100, 100, WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE);
	while (engine->running())
	{
		frameStart = SDL_GetTicks();
		engine->render();

		frameTime = SDL_GetTicks() - frameStart;
		if (frameTime < DELAY_TIME)
		{
			engine->handleEvents();
			engine->update();
		}
	}
	engine->clean();
	return 0;
}