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
#include <entt/entity/registry.hpp>

#include "MovingAverage.hpp"
#include "ShaderProgram.h"
#include "Game.h"

namespace Fling
{
	/**
	 * @brief Core engine class of Fling. This is where the core update loop lives 
	 * along with all startup/shutdown ordering. 
	 */
	class Engine : public NonCopyable
	{
	public:

		FLING_API Engine() = default;

		FLING_API ~Engine() = default;

		/**
		 * @brief Run the engine (Startup, Tick until should stop, and shutdown)
		 * 
		 * @return UINT64 0 for success, otherwise an error has occured
		 */
		template<class T_GameType>
		FLING_API UINT64 Run(ShaderProgram* t_ShaderProgram);

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

		/** Global registry that stores entities and components */
		entt::registry g_Registry;

        /** The shader program will be specified by the end-user for now to make iteration easier */
        ShaderProgram* m_ShaderProgram = nullptr;
	};

	template<class T_GameType>
	FLING_API UINT64 Engine::Run(ShaderProgram* t_ShaderProgram)
	{
		static_assert(std::is_default_constructible<T_GameType>::value, "T_GameType requires default-constructible elements");
		static_assert(std::is_base_of<Fling::Game, T_GameType>::value, "T_GameType must inherit from Fling::Game");

		// #TODO Use a pool allocator for new
		m_GameImpl = new T_GameType();

        m_ShaderProgram = t_ShaderProgram;

		Startup();

		Tick();

		Shutdown();

		return 0;
	}
}	// namespace Fling