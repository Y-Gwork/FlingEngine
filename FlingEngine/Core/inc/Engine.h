#pragma once

#include "Platform.h"
#include "Logger.h"
#include "Timing.h"
#include "ResourceManager.h"
#include "Renderer.h"
#include "FlingConfig.h"
#include "NonCopyable.hpp"
#include "World.h"
#include <nlohmann/json.hpp>

#include "Game.h"

namespace Fling
{
	/**
	 * @brief Core engine class of Fling. This is where the core update loop lives 
	 * along with all startup/shutdown ordering. 
	 */
	class FLING_API Engine : public NonCopyable
	{
	public:

		Engine() = default;

		~Engine() = default;

		/**
		 * @brief Run the engine (Startup, Tick until should stop, and shutdown)
		 * 
		 * @return UINT64 0 for success, otherwise an error has occured
		 */
		template<class T_GameType>
		UINT64 Run();

	private:

		/// <summary>
		/// Start any systems or subsystems that may be needed
		/// </summary>
		void Startup();

		/// <summary>
		/// Initial tick for the engine frame
		/// </summary>
		void Tick();

		/// <summary>
		/// Shutdown all engine systems and do any necessary cleanup
		/// </summary>
		void Shutdown();

		/** Persistent world object that can be used to load levels, entities, etc */
		World* m_World = nullptr;

		Fling::Game* m_GameImpl = nullptr;
	};

	template<class T_GameType>
	UINT64 Engine::Run()
	{
		static_assert(std::is_default_constructible<T_GameType>::value, "T_GameType requires default-constructible elements");
		static_assert(std::is_base_of<Fling::Game, T_GameType>::value, "T_GameType must inherit from Fling::Game");

		// #TODO Use a pool allocator for new
		m_GameImpl = new T_GameType();

		Startup();

		Tick();

		Shutdown();

		return 0;
	}
}	// namespace Fling