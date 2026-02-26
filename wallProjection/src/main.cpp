#include "ofApp.h"
#include "ofAppGLFWWindow.h"
#include "ofMain.h"

int main() {
	ofJson jSettings = ofLoadJson("settings.json");
	ofJson jScreens = jSettings["screens"];

	// creates windows

	ofGLFWWindowSettings settings;

	// main window
	settings.setSize(jScreens[0]["size"][0], jScreens[0]["size"][1]);
	settings.monitor = jScreens[0].value("monitor", 0);
	settings.windowMode = jScreens[0].value("fullscreen", false) ? OF_FULLSCREEN : OF_WINDOW;
	settings.resizable = true;
	settings.decorated = false;
	settings.title = jScreens[0]["id"].get<string>();
	auto mainWindow = ofCreateWindow(settings);
	auto mainApp = make_shared<ofApp>();
	vector<std::unique_ptr<of::priv::AbstractEventToken>> listeners;

	// additional windows
	for (int i = 1; i < (int)jScreens.size(); ++i) {
		if (jScreens[i]["screenType"].get<string>() == "screen") {

			settings.setSize(jScreens[i]["size"][0], jScreens[i]["size"][1]);
			settings.monitor = jScreens[i].value("monitor", i);
			settings.windowMode = jScreens[i].value("fullscreen", false) ? OF_FULLSCREEN : OF_WINDOW;
			settings.shareContextWith = mainWindow;
			settings.title = jScreens[i]["id"].get<string>();

			auto window = ofCreateWindow(settings);
			window->setVerticalSync(false);

			mainApp->extraWindows.push_back(window);
			ofApp * app = mainApp.get();
			listeners.push_back(window->events().draw.newListener([app, i](ofEventArgs & args) { app->drawWindow(i, args); }));
			listeners.push_back(window->events().keyPressed.newListener([app, i](ofKeyEventArgs & args) { app->keyPressedWindow(i, args); }));
		}
	}

	//debug window
	if (jSettings["debug"]["debugScreen"].get<bool>() == true) {
		settings.setSize(1920, 1080);
		settings.monitor = jSettings["debug"].value("monitor", 0);
		settings.windowMode = OF_WINDOW;
		settings.resizable = true;
		settings.decorated = true;
		settings.shareContextWith = mainWindow;
		settings.title = "debug";

		auto window = ofCreateWindow(settings);
		window->setVerticalSync(false);

		ofApp * app = mainApp.get();
		ofAddListener(window->events().draw, mainApp.get(), &ofApp::drawDebugWindow);
		ofAddListener(window->events().keyPressed, mainApp.get(), &ofApp::keyPressedDebugWindow);
	}

	ofRunApp(mainWindow, mainApp);
	ofRunMainLoop();
}
