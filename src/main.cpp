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
		GLFWwindow* window = Framework.getWindow();
		tx::glfwSetKeyCallback<Application, &Application::onKeyEvent>(Framework.getWindow(), this);
		
		glfwSetWindowUserPointer(window, this); 
        glfwSetMouseButtonCallback(window, &Application::onMouseButton);
        glfwSetCursorPosCallback(window, &Application::onMouseMove);
		
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

	static void onMouseButton(GLFWwindow* window, int button, int action, int mods) {
        // Get the "Application" instance from the window
        Application* app = (Application*)glfwGetWindowUserPointer(window);
        
        if (app && button == GLFW_MOUSE_BUTTON_LEFT) {
            double x, y;
            glfwGetCursorPos(window, &x, &y);
            
            int w, h;
            glfwGetWindowSize(window, &w, &h);
            
            // Send to Game: isDown = Press, isRelease = Release
            app->game.onMouseEvent((float)x, (float)y, (action == GLFW_PRESS), (action == GLFW_RELEASE), w, h);
        }
    }

    static void onMouseMove(GLFWwindow* window, double x, double y) {
        Application* app = (Application*)glfwGetWindowUserPointer(window);
        if (app) {
            int w, h;
            glfwGetWindowSize(window, &w, &h);
            
            // Send movement updates (isDown and isRelease are false here)
            app->game.onMouseEvent((float)x, (float)y, false, false, w, h);
        }
    }

  private:
	Game game;


	void update() {
		game.update();
	}
	void render() {
		//tx::Time::Timer timer;
		game.render();
		//cout << timer.duration() << "ms" << endl;
	}
};

int main() {
	if (!glfwInit()) {
		cout << "[FatalError]: Failed to init GLFW\n";
		return 0;
	}
	cout << "Initializing Application...\n";
	Application app;
	cout << "[Status]: Successfully initialized Application.\n";
	app.run();
	return 0;
}