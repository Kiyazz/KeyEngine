/* Hey Student --
 * Take a look at some of the public static variables below-- you may use them to help you debug.
 */

#ifndef INPUTHELPER_H
#define INPUTHELPER_H

#define HELPER_VERSION 0.91

#include <queue>
#include <fstream>
#include <cstdlib>
#include <algorithm>
#include <random>

#include "SDL.h"

/* The Helper class contains mostly static functions / data, and doesn't need to be instanced. */
/* Call the public static functions below via Helper::<function>() */
class Helper {
public:

	/* The frame_number advances with every call to Helper::SDL_RenderPresent() */
	static inline int frame_number = 0;
	static inline Uint32 current_frame_start_timestamp = 0;
	static int GetFrameNumber() { return frame_number; }

	/* Wrapper that renders to screen while also persisting to a .BMP file */
	static void SDL_RenderPresent(SDL_Renderer* renderer)
	{
		::SDL_RenderPresent(renderer);
		SDL_Delay();
		frame_number++;
	}

	static void SDL_RenderCopyEx(int actor_id, const std::string& actor_name, SDL_Renderer* renderer, SDL_Texture* texture, const SDL_FRect* srcrect, const SDL_FRect* dstrect, const float angle, const SDL_FPoint* center, const SDL_RendererFlip flip)
	{
		SDL_Rect* srcrect_i_ptr = nullptr;
		SDL_Rect* dstrect_i_ptr = nullptr;
		SDL_Point* center_i_ptr = nullptr;

		SDL_Rect srcrect_i, dstrect_i;

		SDL_Point center_i;

		if (srcrect != nullptr)
		{
			srcrect_i = { static_cast<int>(srcrect->x), static_cast<int>(srcrect->y), static_cast<int>(srcrect->w), static_cast<int>(srcrect->h) };
			srcrect_i_ptr = &srcrect_i;
		}

		if (dstrect != nullptr)
		{
			dstrect_i = { static_cast<int>(dstrect->x), static_cast<int>(dstrect->y), static_cast<int>(dstrect->w), static_cast<int>(dstrect->h) };
			dstrect_i_ptr = &dstrect_i;
		}

		if (center != nullptr)
		{
			center_i = { static_cast<int>(center->x), static_cast<int>(center->y) };
			center_i_ptr = &center_i;
		}

		/* Perform the render like normal. */
		::SDL_RenderCopyEx(renderer, texture, srcrect_i_ptr, dstrect_i_ptr, angle, center_i_ptr, flip);
	}

	/* This encourages students to keep their data in float form as long as possible. We handle truncating to ints at the very end for them, as necessary. */
	static void SDL_RenderCopy(SDL_Renderer* renderer, SDL_Texture* texture, const SDL_FRect * srcrect, const SDL_FRect * dstrect)
	{
		SDL_Rect* srcrect_i_ptr = nullptr;
		SDL_Rect* dstrect_i_ptr = nullptr;

		SDL_Rect srcrect_i, dstrect_i;

		if (srcrect != nullptr)
		{
			srcrect_i = { static_cast<int>(srcrect->x), static_cast<int>(srcrect->y), static_cast<int>(srcrect->w), static_cast<int>(srcrect->h) };
			srcrect_i_ptr = &srcrect_i;
		}

		if (dstrect != nullptr)
		{
			dstrect_i = { static_cast<int>(dstrect->x), static_cast<int>(dstrect->y), static_cast<int>(dstrect->w), static_cast<int>(dstrect->h) };
			dstrect_i_ptr = &dstrect_i;
		}

		::SDL_RenderCopy(renderer, texture, srcrect_i_ptr, dstrect_i_ptr);
	}

	static void SDL_QueryTexture(SDL_Texture* texture, float* w, float* h)
	{
		if (texture == nullptr)
			return;

		int w_i, h_i;
		::SDL_QueryTexture(texture, nullptr, nullptr, &w_i, &h_i);
		if (w != nullptr)
			*w = static_cast<float>(w_i);
		if (h != nullptr)
			*h = static_cast<float>(h_i);
	}

private:

	/* The engine will aim for 60fps (16ms per frame) during a normal play session. */
	/* If the engine detects it is being autograded, it will run as fast as possible. */
	static void SDL_Delay() {


			Uint32 current_frame_end_timestamp = SDL_GetTicks();  // Record end time of the frame
			Uint32 current_frame_duration_milliseconds = current_frame_end_timestamp - current_frame_start_timestamp;
			Uint32 desired_frame_duration_milliseconds = 16;

			int delay_ticks = std::max(static_cast<int>(desired_frame_duration_milliseconds) - static_cast<int>(current_frame_duration_milliseconds), 1);

			::SDL_Delay(delay_ticks);


		current_frame_start_timestamp = SDL_GetTicks();  // Record start time of the frame
	}
};

/* Utilized for homework 9 */
class RandomEngine
{
	std::default_random_engine engine;
	std::uniform_real_distribution<float> distribution;

public:
	/* Please look up the correct seed value for your usage purposes. Find it in the homework9 assignment spec. */
	RandomEngine(float min, float max, int seed_look_up_in_assignment_spec_pls) //NOLINT random engine seeding
	{
		Configure(min, max, seed_look_up_in_assignment_spec_pls);
	}

	/* Please look up the correct seed value for your usage purposes. Find it in the homework9 assignment spec. */
	void Configure(float min, float max, int seed_look_up_in_assignment_spec_pls)
	{
		engine = std::default_random_engine(seed_look_up_in_assignment_spec_pls);
		distribution = std::uniform_real_distribution<float>(min, max);

	}

	/* If you use this constructor, don't forget to configure the engine later. */
	RandomEngine() = default;

	/* Get a random number in the specified range. */
	float Sample()
	{
		return distribution(engine);
	}

	void discard(size_t z) {
		engine.discard(z);
	}
};

#endif
