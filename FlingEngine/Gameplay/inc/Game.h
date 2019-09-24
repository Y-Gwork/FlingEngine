#pragma once

#include "NonCopyable.hpp"
#include <string>

namespace Fling
{
	/**
	 * @brief   The game class is mean to be overridden on a per-game instance.
	 *          It provides an interface for users to add their own System calls
	 *          in the update, read, write, etc
	 * @see World
	 */
	class Game : public NonCopyable
	{
		friend class Engine;

	public:
		
		/**
		 * TODO
		 */
		virtual void Init() = 0;

		/* Called when the engine is shutting down */
		virtual void Shutdown() = 0;

		/**
		* Update is called every frame. Call any system updates for your gameplay systems inside of here
		*/
		virtual void Update(float DeltaTime) = 0;
		
		/**
		 * Read is called when a level file is read from disk. Here is where you can 
		 * read in a file stream and based the data inside it, initalize your gameplay
		 * systems
		 */
		virtual bool Read(/* TODO: Cereal stream */) = 0;
		
		/**
		 * TODO
		 */
		virtual bool Write(/* TODO: Cereal stream  */) = 0;

	protected:

		// You really should not be implementing the game's ctor
		Game() = default;
		virtual ~Game() = default;
	};
}   // namespace Fling