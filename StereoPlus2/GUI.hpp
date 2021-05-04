#pragma once
#include "GLLoader.hpp"
#include "Commands.hpp"
#include "Window.hpp"
#include "Windows.hpp"
#include "Input.hpp"
#include "Localization.hpp"
#include <map>


class GUI {
#pragma region Private

	const Log log = Log::For<GUI>();
	bool shouldClose = false;

	FileWindow* fileWindow = nullptr;

	bool CreateFileWindow(FileWindow::Mode mode) {
		auto fileWindow = new FileWindow();

		fileWindow->mode = mode;
		fileWindow->BindScene(scene);

		if (!fileWindow->Init())
			return false;

		windows.push_back((Window*)fileWindow);

		this->fileWindow = fileWindow;
		fileWindow->OnExit().AddHandler([f = &this->fileWindow]{
			(new FuncCommand())->func = [f = f] {
				delete* f;
				*f = nullptr;
				};
			});
		
		return true;
	}

	bool OpenFileWindow(FileWindow::Mode mode) {
		if (fileWindow != nullptr)
			fileWindow->mode = mode;
		else if (!CreateFileWindow(mode))
			return false;

		return true;
	}

	bool DesignMenuBar() {
		if (ImGui::BeginMenu(LocaleProvider::GetC("file"))) {
			if (ImGui::MenuItem(LocaleProvider::GetC("open"), nullptr, false))
				if (!OpenFileWindow(FileWindow::Load))
					return false;
			if (ImGui::MenuItem(LocaleProvider::GetC("save"), nullptr, false))
				if (!OpenFileWindow(FileWindow::Save))
					return false;
			if (ImGui::MenuItem(LocaleProvider::GetC("close"), nullptr, false))
				scene->DeleteAll();

			if (auto h = Settings::ShouldDetectPosition().Get(); 
				ImGui::MenuItem(LocaleProvider::GetC("usePositionDetection"), nullptr, &h))
				Settings::ShouldDetectPosition() = h;
			ImGui::MenuItem(LocaleProvider::GetC("showFPS"), nullptr, &shouldShowFPS);

			if (ImGui::MenuItem(LocaleProvider::GetC("settings"), nullptr, false))
				settingsWindow->IsOpen = true;

			if (ImGui::MenuItem(LocaleProvider::GetC("exit"), nullptr, false))
				shouldClose = true;

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(LocaleProvider::GetC("render"))) {
			if (ImGui::MenuItem(LocaleProvider::GetC("renderViewport"), "F5", false))
				renderViewport();
			if (ImGui::MenuItem(LocaleProvider::GetC("renderAdvanced"), "F6", false))
				renderAdvanced();

			ImGui::EndMenu();
		}

		if (shouldShowFPS) {
			ImGui::LabelText("", "FPS: %-12i DeltaTime: %-12f", Time::GetAverageFrameRate(), Time::GetAverageDeltaTime());
		}

		return true;
	}

	bool DesignMainWindowDockingSpace()
	{
		// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
		// because it would be confusing to have two docking targets within each others.
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
		// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
		// all active windows docked into it will lose their parent and become undocked.
		// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
		// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		
		// Make the window behind docking space transparent 
		// to enable transparency for CustomRenderWindow when docked.
		ImGui::PushStyleColor(ImGuiCol_WindowBg, glm::vec4());

		// Main window docking space cannot be closed.
		bool open = true;
		ImGui::Begin("MainWindowDockspace", &open, window_flags);

		ImGui::PopStyleVar(3);
		ImGui::PopStyleColor();

		// DockSpace
		ImGuiIO& io = ImGui::GetIO();
		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

		if (ImGui::BeginMenuBar()) {
			if (!DesignMenuBar())
				return false;

			ImGui::EndMenuBar();
		}

		ImGui::End();

		return true;
	}

#pragma endregion
public:
	// It seems to be garbage collected or something.
	// When trying to free it it fails some imgui internal free function.
	ImGuiIO* io;
	const char* glsl_version;
	GLFWwindow* glWindow;
	Input input;
	KeyBinding keyBinding;
	Scene* scene;

	SettingsWindow* settingsWindow;

	bool shouldShowFPS = true;

	std::vector<Window*> windows;
	std::function<bool()> customRenderFunc;
	std::function<void()> renderViewport;
	std::function<void()> renderAdvanced;

	bool Init()
	{
		keyBinding.input = &input;
		input.GLFWindow() = glWindow;

		
		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		io = &ImGui::GetIO();
		//io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
		io->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
		//io.ConfigViewportsNoAutoMerge = true;
		//io.ConfigViewportsNoTaskBarIcon = true;

		// Setup Dear ImGui style
		ImGui::Extensions::StyleColorsStereo();
		//ImGui::StyleColorsDark();
		//ImGui::StyleColorsClassic();

		// Setup Platform/Renderer bindings
		ImGui_ImplGlfw_InitForOpenGL(glWindow, true);
		ImGui_ImplOpenGL3_Init(glsl_version);

		// Load Fonts
		// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
		// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
		// - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
		// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
		// - Read 'misc/fonts/README.txt' for more instructions and details.
		// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !


		ImFontConfig font_config;
		font_config.OversampleH = 1; //or 2 is the same
		font_config.OversampleV = 1;
		font_config.PixelSnapH = 1;

		static const ImWchar ranges[] =
		{
			0x0020, 0x00FF, // Basic Latin + Latin Supplement
			0x0400, 0x04FF, // Cyrillic
			0x0500, 0x052F, // Cyrillic supplement
			0,
		};


		io->Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Tahoma.ttf", 14.0f, &font_config, ranges);

		//io.Fonts->AddFontDefault();
		//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
		//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
		//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
		//io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
		//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
		//IM_ASSERT(font != NULL);


		//ImGuiIO& io = ImGui::GetIO();
		//ImFont* font = io.Fonts->AddFontFromFileTTF("open-sans.ttf", 20);
		//IM_ASSERT(font != NULL);

		input.io = io;
		if (!input.Init() ||
			!keyBinding.Init())
			return false;

		for (auto window : windows)
			if (!window->Init())
				return false;

		return true;
	}

	bool Design()
	{
		// Show main window docking space
		if (!DesignMainWindowDockingSpace())
			return false;

		if (shouldClose)
			return true;

		for (Window* window : windows)
			if (window->ShouldClose()) {
				if (!window->Exit())
					return false;
				else
					windows.erase(std::find(windows.begin(), windows.end(), window));
			}
			else if (!window->Design())
				return false;

		return true;
	}

	bool MainLoop() {
		// Main loop
		while (!glfwWindowShouldClose(glWindow)) {
			// Poll and handle events (inputs, window resize, etc.)
			// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
			// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
			// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
			// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
			glfwPollEvents();
			input.ProcessInput();

			// Start the Dear ImGui frame
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			if (!Design())
				return false;

			if (shouldClose)
				return true;

			// Rendering
			ImGui::Render();
			int display_w, display_h;
			glfwGetFramebufferSize(glWindow, &display_w, &display_h);
			glViewport(0, 0, display_w, display_h);
			glClear(GL_COLOR_BUFFER_BIT);
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			// Update and Render additional Platform Windows
			// (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
			//  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
			if (io->ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
				GLFWwindow* backup_current_context = glfwGetCurrentContext();
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
				glfwMakeContextCurrent(backup_current_context);
			}

			glfwSwapBuffers(glWindow);

			if (!Command::ExecuteAll())
				return false;

			Time::UpdateFrame();
			//std::cout << "FPS: " << Time::GetFrameRate() << std::endl;
		}

		return true;
	}
	
	bool OnExit()
	{
		for (Window* window : windows)
			if (!window->Exit())
				return false;
		
		// Cleanup
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		glfwDestroyWindow(glWindow);
		glfwTerminate();

		return true;
	}
};

