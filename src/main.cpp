#include "Project.hpp"

class Application {
  private:
	struct UpdateFunc {
		Application* ptr;
		inline void operator()() {
			ptr->update();
		}
	};
	struct RenderFunc {
		Application* ptr;
		inline void operator()() {
			ptr->render();
		}
	};
	tx::RE::Framework<tx::RE::Mode::Release, UpdateFunc, RenderFunc> Framework{UpdateFunc{this}, RenderFunc{this}};

  public:
	void run() {
		this->Framework.run();
	}

  public:
	Application() {
		tx::glfwSetKeyCallback<Application, &Application::onKeyEvent>(Framework.getWindow(), this);
		tx::glEnableTransparent();
	}
	~Application() {

	}

  private:
	void onKeyEvent(GLFWwindow* window, int key, int scancode, int action, int mods) {
		if (action == GLFW_PRESS || action == GLFW_REPEAT) {
			switch (key) {
				
			}
		}
	}

  private:
	Game game;


	void update() {

	}
	void render() {
		game.render();
	}
};

int main() {
	if (!glfwInit()) {
		cout << "[FatalError]: Failed to init GLFW\n";
		return 0;
	}
	Application app;
	cout << "[Status]: Successfully initialized window.\n";
	app.run();
	return 0;
}