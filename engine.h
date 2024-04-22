#include "SDL2/SDL.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

#include <iostream>
#include <vector>
#include <cmath>
#include <complex>

#define TYPE double
#define STARTRATIO 5.0
#define MAX_W 3840
#define MAX_H 2160

using std::complex;
using namespace std::complex_literals;

struct RGB {
	Uint8 r, g, b;
};

class coordinate {
public:
	TYPE x, y;
	coordinate() {
		x = 0;
		y = 0;
	}
	coordinate(TYPE x1, TYPE y1) {
		x = x1;
		y = y1;
	};
};

coordinate trasformazione(coordinate centro, coordinate valore, TYPE ratio) {
	coordinate temp(valore.x,valore.y);

	temp.x -= centro.x;
	temp.y -= centro.y;

	temp.x *= ratio;
	temp.y *= ratio;

	temp.x += centro.x;
	temp.y += centro.y;

	return temp;
}

class Pixel
{
public:
	complex <TYPE> z;
	int divergenceNum;

	complex <TYPE> pixelToComplex(int height_p, int width_p, coordinate pixel, coordinate center, TYPE height, TYPE width) {//normalizza e calcola z a partire da pixel e schermo
		coordinate center_p(width_p / 2, height_p / 2);
		pixel.x = (pixel.x - center_p.x) / center_p.x * width + center.x;
		pixel.y = (pixel.y - center_p.y) / center_p.y * height + center.y;
		return (complex <TYPE>(pixel.x, -pixel.y));
	};

	Pixel(){
		z = (0,0);
	};

	void init_p(int height_p, int width_p, coordinate pixel, coordinate center, TYPE height, TYPE width) {
		z = pixelToComplex(height_p, width_p, pixel, center, height, width);
		divergenceNum = -1;
	}

	coordinate complexToCoordinates() {
		coordinate temp(z.real(), z.imag());
		return temp;
	}

	void mandelbrot_iteration(int height_p, int width_p, coordinate pixel, coordinate center, TYPE height, TYPE width) {
		z = pow(z, 2) + pixelToComplex(height_p, width_p, pixel, center, height, width);
	};
	
	RGB NumberToRGB(int convergenza) { //numero - soglia di convergenza
		//per sicurezza si imposta il massimo a convergenza
		// Interpola RGB in base al valore del modulo di z
		Uint8 r;
		Uint8 g;
		Uint8 b;
		if (divergenceNum == -1)
			r = g = b = 0;
		else {
			r = static_cast<Uint8>((1.0 - (((TYPE)convergenza) - divergenceNum) / ((TYPE)convergenza)) * 255.0);
			g = static_cast<Uint8>((1.0 - (((TYPE)convergenza) - divergenceNum) / ((TYPE)convergenza)) * 255.0);
			b = static_cast<Uint8>((1.0 - (((TYPE)convergenza) - divergenceNum) / ((TYPE)convergenza)) * 255.0);
		}

		return { r, g, b };
	};
};



class Piano {
public:
	SDL_Surface* surface;
	int width, height;


	void reset(SDL_Surface* S) {
		surface = S;
		width = surface->w;
		height = surface->h;
	};

	void setPixel(RGB color, int x, int y) {
		Uint32* target_pixel = (Uint32*)((Uint8*)surface->pixels
			+ y * surface->pitch
			+ x * surface->format->BytesPerPixel);
		*target_pixel = SDL_MapRGB(surface->format, color.r, color.g, color.b);
	}
};

Pixel p[MAX_H][MAX_W];

class Engine
{
public:
	bool Running;
	int windowX, windowY;

	TYPE maxX, maxY;
	TYPE ratio;
	TYPE transitionState;

	bool generating;
	bool resized;
	bool drawn, linedrawn;

	unsigned iteration;
	unsigned max_iteration;

	coordinate center;
	int currentY;

	SDL_Window* Window;
	SDL_Renderer* Renderer;
	SDL_Surface* Surface;
	SDL_Texture* Texture;
	Piano* piano = new Piano;

	int zoomstate;
	int mousex, mousey;
	int drawx, drawy;

	Engine() {};

	bool init(const char* title, int xpos, int ypos, int width, int height, int flags)//prepara SDL
	{
		if (SDL_Init(SDL_INIT_EVERYTHING) == 0)
		{
			Window = SDL_CreateWindow(title, xpos, ypos, width, height, flags);
			if (Window != 0)
			{
				Renderer = SDL_CreateRenderer(Window, -1, 0);
				if (Renderer != 0)
				{
					SDL_SetRenderDrawColor(Renderer, 0, 0, 0, 255);
					SDL_GetRendererOutputSize(Renderer, &windowX, &windowY);
				}
				else
				{
					return false;
				}
			}
			else
			{
				std::cout << "window init fail\n";
				return false;
			}
		}
		else
		{
			std::cout << "SDL init fail\n";
			return false;
		}
		std::cout << "init success\n";
		Running = true;

		windowX = width;
		windowY = height;

		generating = true;
		resized = true;
		drawn = false;
		linedrawn = false;

		max_iteration = 100;
		ratio = STARTRATIO;
		transitionState = ratio;
		maxX = ratio;
		maxY = ratio / windowX * windowY;
		center.x = center.y = 0;
		Surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBX8888);
		piano->reset(Surface);
		currentY = 0;
		zoomstate = 0;
		init_screen();
		return true;
	};

	void init_screen() {
		coordinate temp(0, 0);
		for (int y = 0; y < windowY; y++) {
			temp.y = y;
			for (int x = 0; x < windowX; x++) {
				temp.x = x;
				p[y][x].init_p(windowY, windowX, temp, center, maxY, maxX);
			}
		}
		drawx = drawy = 0;
	};


	void initSurface() {
		Surface = SDL_GetWindowSurface(Window);
	}

	void handleEvents() {//registra i tasti cliccati
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym)
				{
				case SDLK_ESCAPE:
					Running = false;
					break;
				}
				break;
			case SDL_MOUSEWHEEL: {//se si muove la rotella del mouse si prendono coordinate e direzione
				if (event.wheel.y > 0)
					zoomstate = 1;
				else
					zoomstate = -1;
				SDL_GetMouseState(&mousex, &mousey);
				break;
			}
			case SDL_QUIT:
				Running = false;
				break;
			default:
				break;
			}
		}
	};

	void zoom() {
		if (zoomstate == 0)
			return;

		if (zoomstate == 1) {
			ratio *= 2;
			maxX *= 2;
			maxY *= 2;
		}
		if (zoomstate == -1) {
			ratio /= 2;
			maxX /= 2;
			maxY /= 2;
		}

		center.x = mousex;
		center.y = mousey;
		resized = true;
		drawn = false;
	};

	void resize() {

	}

	void render() {
		if (generating && linedrawn) {
			Texture = SDL_CreateTextureFromSurface(Renderer, Surface);
			SDL_RenderCopy(Renderer, Texture, NULL, NULL);
			SDL_RenderPresent(Renderer);
			SDL_DestroyTexture(Texture);
			linedrawn = false;
		}
		else if (drawn) {
			Texture = SDL_CreateTextureFromSurface(Renderer, Surface);
			SDL_RenderCopy(Renderer, Texture, NULL, NULL);
			SDL_RenderPresent(Renderer);
			SDL_DestroyTexture(Texture);
			drawn = false;
		}
		
	};


	void update() {
		if (drawx > windowX) {
			drawx = 0;
			drawline(drawy);
			drawy++;
		}
		if (drawy >= windowY) {
			generating = false;
			drawx = 0;
			drawy = 0;
			drawn = true;
		}
		if (generating) {
			for (int i = 0; i < max_iteration; i++) {
				p[drawy][drawx].mandelbrot_iteration(windowY, windowX, (coordinate(drawx,drawy)), center, maxY, maxX);
				if (abs(p[drawy][drawx].z) > 100) {
					p[drawy][drawx].divergenceNum = i;
					break;
				}
			}
			drawx++;
			return;
		}
	};

	void drawline(int y) {
		SDL_LockSurface(piano->surface);
		for(int x=0;x<windowX;x++)
			piano->setPixel(p[y][x].NumberToRGB(max_iteration), x, y);
		SDL_UnlockSurface(piano->surface);
		linedrawn = true;
	};
	void debug() {
		
	}

	void clean() {//chiude tutto
		std::cout << "cleaning\n";
		SDL_DestroyWindow(Window);
		SDL_DestroyRenderer(Renderer);
		SDL_Quit();
	};

	bool running() { return Running; }
};