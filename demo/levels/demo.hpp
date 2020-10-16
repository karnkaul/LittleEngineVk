#include <engine/game/freecam.hpp>
#include <engine/game/state.hpp>
#include <level.hpp>

struct FPS final {
	le::Time updated;
	le::Time elapsed;
	le::u32 fps = 0;
	le::u32 frames = 0;
	bool bSet = false;

	le::u32 update() {
		using namespace std::chrono_literals;
		if (updated > le::Time()) {
			elapsed += (le::Time::elapsed() - updated);
		}
		updated = le::Time::elapsed();
		++frames;
		if (elapsed.duration >= 1s) {
			fps = frames;
			frames = 0;
			elapsed = {};
			bSet = true;
		}
		return bSet ? fps : frames;
	}
};

class DemoLevel : public Level {
  public:
	le::u32 m_fps;

  public:
	DemoLevel();

  protected:
	void tick(le::Time dt) override;
	void onManifestLoaded() override;
	le::SceneBuilder const& builder() const override;

  private:
	struct {
		stdfs::path model0id, model1id, skyboxID;
		le::res::Async<le::res::Model> asyncModel0, asyncModel1;
		le::gfx::DirLight dirLight0, dirLight1;
		le::Prop eid0, eid1, eid2, eid3, eui0;
		le::ecs::Entity eui1, eui2;
		le::Prop pointer;
		le::input::Context temp;
		le::FreeCam freeCam;
		FPS fps;
		le::Token tempToken;
		le::Time reloadTime;
		bool bLoadUnloadModels = false;
		bool bWireframe = false;
		bool bQuit = false;
	} m_data;
	struct {
		le::res::TScoped<le::res::Material> texturedLit;
		le::res::TScoped<le::res::Mesh> triangle;
		le::res::TScoped<le::res::Mesh> quad;
		le::res::TScoped<le::res::Mesh> sphere;
		le::Hash container2 = "textures/container2.png";
		le::Hash container2_specular = "textures/container2_specular.png";
		le::Hash awesomeface = "textures/awesomeface.png";
	} m_res;
	le::SceneBuilder m_sceneBuilder;
};
