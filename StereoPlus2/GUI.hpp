#pragma once
#include "GLLoader.hpp"
#include "Window.hpp"
#include <map>

struct KeyStatus {
	bool isPressed = false;
	bool isDown = false;
	bool isUp = false;
};

// First frame keys are never Up or Down
// as we need to check previous state in order to infer those.
class Input
{
	glm::vec2 mouseOldPos = glm::vec2(0);
	glm::vec2 mouseNewPos = glm::vec2(0);

	bool isMouseBoundlessMode = false;
	bool isRawMouseMotionSupported = false;

	std::map<GLuint, KeyStatus*> keyStatuses;

	void UpdateStatus(GLuint key, KeyStatus* status) {
		bool isPressed = glfwGetKey(window, key) == GLFW_PRESS;

		status->isDown = isPressed && !status->isPressed;
		status->isUp = !isPressed && status->isPressed;
		status->isPressed = isPressed;
	}

	void EnsureKeyStatusExists(GLuint key) {
		if (keyStatuses.find(key) != keyStatuses.end())
			return;

		KeyStatus* status = new KeyStatus();
		keyStatuses.insert({ key, status });
		UpdateStatus(key, status);
	}


public:
	GLFWwindow* window;

	std::vector<std::function<void()>> handlers;


	bool IsPressed(GLuint key)
	{
		EnsureKeyStatusExists(key);

		return keyStatuses[key]->isPressed;
	}

	bool IsDown(GLuint key)
	{
		EnsureKeyStatusExists(key);

		return keyStatuses[key]->isDown;
	}

	bool IsUp(GLuint key)
	{
		EnsureKeyStatusExists(key);

		return keyStatuses[key]->isUp;
	}


	glm::vec2 MousePosition() {
		return mouseNewPos;
	}

	glm::vec2 MouseMoveDirection() {
		return glm::length(mouseNewPos - mouseOldPos) == 0 ? glm::vec2(0) : glm::normalize(mouseNewPos - mouseOldPos);
	}

	float MouseSpeed() {
		return glm::length(mouseNewPos - mouseOldPos);
	}

	void SetMouseBoundlessMode(bool enable) {
		if (enable == isMouseBoundlessMode)
			return;
	
		if (enable)
		{
			if (isRawMouseMotionSupported)
				glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}
		else
		{
			// We can disable raw mouse motion mode even if it's not supported
			// so we don't bother with checking it.
			glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);

			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}

		isMouseBoundlessMode = enable;
	}

	void ProcessInput()
	{
		// Update mouse position
		mouseOldPos = mouseNewPos;
		mouseNewPos = ImGui::GetMousePos();

		// Update key statuses
		for (auto node : keyStatuses)
		{
			UpdateStatus(node.first, node.second);
		}

		// Handle OnInput actions
		for (auto handler : handlers)
		{
			handler();
		}
	}

	bool Init() {
		isRawMouseMotionSupported = glfwRawMouseMotionSupported();

		return true;
	}

	~Input() {
		for (auto node : keyStatuses)
		{
			delete node.second;
		}
	}
};

class KeyBinding
{
	void AddHandler(std::function<void()> func) {
		input->handlers.push_back(func);
	}


	bool isAxeModeEnabled;
public:
	Input* input;
	Cross* cross;

	float crossMovementSpeed = 0.01;

	void MoveCross() {
		// Simple mouse control
		//AddHandler([i = input, c = cross, sp = crossMovementSpeed] {
		//	bool isAltPressed = i->IsPressed(GLFW_KEY_LEFT_ALT) || i->IsPressed(GLFW_KEY_RIGHT_ALT);

		//	// Enable or disable Mouose boundless mode 
		//	// regardless of whether we move the cross or not.
		//	i->SetMouseBoundlessMode(isAltPressed);

		//	if (isAltPressed) {
		//		bool isHighPrecisionMode = i->IsPressed(GLFW_KEY_LEFT_CONTROL);

		//		auto m = i->MouseMoveDirection() * sp * (isHighPrecisionMode ? 0.1f : 1);
		//		c->Position.x += m.x;
		//		c->Position.y -= m.y;
		//		c->Refresh();
		//	}
		//});

#pragma region Advanced mouse+keyboard control

		// Axe mode switch A
		AddHandler([i = input, c = cross, sp = crossMovementSpeed, &axeMode = isAxeModeEnabled] {
			if (i->IsDown(GLFW_KEY_A))
			{
				axeMode = !axeMode;
			}
		});

		// Advanced mouse+keyboard control
		AddHandler([i = input, c = cross, sp = crossMovementSpeed, &axeMode = isAxeModeEnabled] {
			if (axeMode)
			{
				// alt x y
				// ctrl x z
				// shift y z

				glm::vec3 axes = glm::vec3(
					(i->IsPressed(GLFW_KEY_LEFT_ALT) || i->IsPressed(GLFW_KEY_RIGHT_ALT)) + (i->IsPressed(GLFW_KEY_LEFT_CONTROL) || i->IsPressed(GLFW_KEY_RIGHT_CONTROL)),
					(i->IsPressed(GLFW_KEY_LEFT_ALT) || i->IsPressed(GLFW_KEY_RIGHT_ALT)) + (i->IsPressed(GLFW_KEY_LEFT_SHIFT) || i->IsPressed(GLFW_KEY_RIGHT_SHIFT)),
					(i->IsPressed(GLFW_KEY_LEFT_CONTROL) || i->IsPressed(GLFW_KEY_RIGHT_CONTROL)) + (i->IsPressed(GLFW_KEY_LEFT_SHIFT) || i->IsPressed(GLFW_KEY_RIGHT_SHIFT))
				);

				bool isZero = axes.x == 0 && axes.y == 0 && axes.z == 0;
				// If all three keys are down then it's impossible to move anywhere.
				bool isBlocked = axes.x == 2 && axes.y == 2 && axes.z == 2;

				bool mustReturn = isBlocked || isZero;

				// Enable or disable Mouose boundless mode 
				// regardless of whether we move the cross or not.
				i->SetMouseBoundlessMode(!mustReturn);

				if (mustReturn)
				{
					return;
				}

				bool isAxeLocked = axes.x == 2 || axes.y == 2 || axes.z == 2;
				if (isAxeLocked)
				{
					int lockedAxeIndex = axes.x == 2 ? 0 : axes.y == 2 ? 1 : 2;

					// Cross position.
					float* destination = &c->Position[lockedAxeIndex]; 

					axes -= 1;

					// Enable or disable Mouose boundless mode 
					// regardless of whether we move the cross or not.
					i->SetMouseBoundlessMode(true);

					auto m = i->MouseMoveDirection() * sp;

					*destination += m.x;

					c->Refresh();

					return;
				}
				
				int lockedPlane[2];
				{
					int i = 0;
					if (axes.x != 0) lockedPlane[i++] = 0;
					if (axes.y != 0) lockedPlane[i++] = 1;
					if (axes.z != 0) lockedPlane[i++] = 2;
				}

				// Enable or disable Mouse boundless mode 
				// regardless of whether we move the cross or not.
				i->SetMouseBoundlessMode(true);

				auto m = i->MouseMoveDirection() * sp;

				c->Position[lockedPlane[0]] += m.x;
				c->Position[lockedPlane[1]] -= m.y;

				c->Refresh();

				return;
			}
			
			bool isAltPressed = i->IsPressed(GLFW_KEY_LEFT_ALT) || i->IsPressed(GLFW_KEY_RIGHT_ALT);

			// Enable or disable Mouose boundless mode 
			// regardless of whether we move the cross or not.
			i->SetMouseBoundlessMode(isAltPressed);

			if (isAltPressed) {
				bool isHighPrecisionMode = i->IsPressed(GLFW_KEY_LEFT_CONTROL);

				auto m = i->MouseMoveDirection() * sp * (isHighPrecisionMode ? 0.1f : 1);
				c->Position.x += m.x;
				c->Position.y -= m.y;
				c->Refresh();
			}
		});

#pragma endregion

		// Move cross with arrows/arrows+Ctrl
		AddHandler([i = input, c = cross, sp = crossMovementSpeed] {
			glm::vec2 movement = glm::vec2(
				-i->IsPressed(GLFW_KEY_LEFT) + i->IsPressed(GLFW_KEY_RIGHT),
				-i->IsPressed(GLFW_KEY_UP) + i->IsPressed(GLFW_KEY_DOWN));

			if (movement.x != 0 || movement.y != 0)
			{
				bool isHighPrecisionMode = i->IsPressed(GLFW_KEY_LEFT_CONTROL);

				movement *= sp * (isHighPrecisionMode ? 0.1f : 1);

				c->Position.x += movement.x;
				c->Position.y -= movement.y;
				c->Refresh();
			}
		});

		// Move cross with numpad/numpad+Ctrl
		AddHandler([i = input, c = cross, sp = crossMovementSpeed] {
			glm::vec3 movement = glm::vec3(
				-i->IsPressed(GLFW_KEY_KP_4) + i->IsPressed(GLFW_KEY_KP_6),
				-i->IsPressed(GLFW_KEY_KP_2) + i->IsPressed(GLFW_KEY_KP_8),
				-i->IsPressed(GLFW_KEY_KP_1) + i->IsPressed(GLFW_KEY_KP_9));

			if (movement.x != 0 || movement.y != 0 || movement.z != 0)
			{
				bool isHighPrecisionMode = i->IsPressed(GLFW_KEY_LEFT_CONTROL);

				movement *= sp * (isHighPrecisionMode ? 0.1f : 1);

				c->Position += movement;
				c->Refresh();
			}
		});
	}

	bool Init() {
		MoveCross();

		return true;
	}
};

class GUI
{
#pragma region Private
	//process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
	//---------------------------------------------------------------------------------------------------------
	void processInput(GLFWwindow* window)
	{
		input.ProcessInput();
	}
#pragma endregion
public:
	// It seems to be garbage collected or something.
	// When trying to free it it fails some imgui internal free function.
	ImGuiIO* io;
	const char* glsl_version;
	GLFWwindow* window;
	Input input;
	KeyBinding keyBinding;
	SceneConfiguration* sceneConfig;

	std::vector<Window*> windows;
	std::function<bool()> customRenderFunc;
	// TEST TEST TEST
	// Our state
	bool show_demo_window = true;
	bool show_another_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	//glm::vec2 renderSize = glm::vec3(1);



	bool Init()
	{
		keyBinding.input = &input;
		input.window = window;

		if (!input.Init()
			|| !keyBinding.Init())
			return false;

		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		io = &ImGui::GetIO(); (void)io;
		io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
		io->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
		//io.ConfigViewportsNoAutoMerge = true;
		//io.ConfigViewportsNoTaskBarIcon = true;

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsClassic();

		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		ImGuiStyle& style = ImGui::GetStyle();
		if (io->ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		// Setup Platform/Renderer bindings
		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init(glsl_version);

		// Load Fonts
		// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
		// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
		// - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
		// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
		// - Read 'misc/fonts/README.txt' for more instructions and details.
		// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
		//io.Fonts->AddFontDefault();
		//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
		//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
		//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
		//io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
		//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
		//IM_ASSERT(font != NULL);

		return true;
	}



	bool Design()
	{

		for (Window* window : windows)
		{
			if (!window->Design())
			{
				return false;
			}
		}

		//// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
		//if (show_demo_window)
		//	ImGui::ShowDemoWindow(&show_demo_window);

		//// 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
		//{
		//	static float f = 0.0f;
		//	static int counter = 0;

		//	ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

		//	ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
		//	ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
		//	ImGui::Checkbox("Another Window", &show_another_window);

		//	ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
		//	ImGui::ColorEdit3("clear color", (float*)& clear_color); // Edit 3 floats representing a color

		//	if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
		//		counter++;
		//	ImGui::SameLine();
		//	ImGui::Text("counter = %d", counter);

		//	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		//	ImGui::End();
		//}

		//// 3. Show another simple window.
		//if (show_another_window)
		//{
		//	ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
		//	ImGui::Text("Hello from another window!");
		//	if (ImGui::Button("Close Me"))
		//		show_another_window = false;
		//	ImGui::End();
		//}

		return true;
	}

	bool MainLoop()
	{
		// Main loop
		while (!glfwWindowShouldClose(window))
		{
			// Poll and handle events (inputs, window resize, etc.)
			// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
			// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
			// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
			// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
			glfwPollEvents();

			// Start the Dear ImGui frame
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			if (!Design())
			{
				return false;
			}

			// Rendering
			ImGui::Render();
			int display_w, display_h;
			glfwGetFramebufferSize(window, &display_w, &display_h);
			glViewport(0, 0, display_w, display_h);
			glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
			glClear(GL_COLOR_BUFFER_BIT);
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			// Update and Render additional Platform Windows
			// (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
			//  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
			if (io->ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				GLFWwindow* backup_current_context = glfwGetCurrentContext();
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
				glfwMakeContextCurrent(backup_current_context);
			}

			glfwSwapBuffers(window);

			processInput(window);
		}

		return true;
	}
	
	bool OnExit()
	{
		for (Window* window : windows)
		{
			if (!window->OnExit())
			{
				return false;
			}
		}

		// Cleanup
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		glfwDestroyWindow(window);
		glfwTerminate();

		return true;
	}

	GUI() 
	{
	}
};

