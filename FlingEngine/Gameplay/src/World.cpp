#include "pch.h"
#include "World.h"
#include "FlingConfig.h"

namespace Fling
{
    void World::Init()
    {
        F_LOG_TRACE("World Init!");

		// Load the that is specific in the config file
		std::string LevelToLoad = FlingConfig::GetString("Game", "StartLevel");
		
		LoadLevel(LevelToLoad);
    }

    void World::Shutdown()
    {
        F_LOG_TRACE("World shutdown!");
		
		// Unload the current levels
		m_ActiveLevels.clear();
    }

	void World::PreTick()
	{
		F_LOG_TRACE("World PreTick!");
	}

    void World::Update(float t_DeltaTime)
    {
		m_ShouldQuit = (m_ShouldQuit ? m_ShouldQuit : Input::IsKeyDown(KeyNames::FL_KEY_ESCAPE));
		// TODO: Update any _world_ systems 
			// The transforms of objects
    }

	// #TODO: Add a callback func for when the level loading is complete
    void World::LoadLevel(const std::string& t_LevelPath)
    {
		F_LOG_TRACE("World loading level: {}", t_LevelPath);

		// #TODO: Unload the current level? Depends on how we want to do async loading in the future

		m_ActiveLevels.emplace_back(std::make_unique<Level>(t_LevelPath, this));
    }
} // namespace Fling