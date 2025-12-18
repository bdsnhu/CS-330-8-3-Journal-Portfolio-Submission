///////////////////////////////////////////////////////////////////////////////
// viewmanager.h
// ============
// manage the viewing of 3D objects within the viewport
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "ViewManager.h"

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>    
#include <iostream>
// declaration of the global variables and defines
namespace
{
	// Variables for window width and height
	const int WINDOW_WIDTH = 1000;
	const int WINDOW_HEIGHT = 800;
	const char* g_ViewName = "view";
	const char* g_ProjectionName = "projection";

	// camera object used for viewing and interacting with
	// the 3D scene
	Camera* g_pCamera = nullptr;

	// these variables are used for mouse movement processing
	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	bool gFirstMouse = true;

	// time between current frame and last frame
	float gDeltaTime = 0.0f; 
	float gLastFrame = 0.0f;

	// I added the camera movement speed so that it can be adjusted with the mouse scroll.
	float gCameraSpeed = 2.5;

	//Minimum and maximum camera speed limits.
	const float MIN_CAMERA_SPEED = 0.5f;// I added this for the minimum camera speed.
	const float MAX_CAMERA_SPEED = 10.0f;// I added this for the maximum camera speed.

	// the following variable is false when orthographic projection
	// is off and true when it is on
	bool bOrthographicProjection = false;

	// I added this code to Store camera states for switching between projections.
	glm::vec3 perspectivePosition;
	glm::vec3 perspectiveFront;
	glm::vec3 perspectiveUp;
	float perspectiveYaw;
	float perspectivePitch;

	glm::vec3 orthographicPosition;
	glm::vec3 orthographicFront;
	glm::vec3 orthographicUp;
	float orthographicYaw;
	float orthographicPitch;

	bool cameraStatesInitialized = false;

}

/***********************************************************
 *  ViewManager()
 *
 *  The constructor for the class
 ***********************************************************/
ViewManager::ViewManager(
	ShaderManager *pShaderManager)
{
	// initialize the member variables
	m_pShaderManager = pShaderManager;
	m_pWindow = NULL;
	g_pCamera = new Camera();
	// default camera view parameters
	g_pCamera->Position = glm::vec3(0.0f, 5.0f, 12.0f);
	g_pCamera->Front = glm::vec3(0.0f, -0.5f, -2.0f);
	g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
	g_pCamera->Zoom = 80;

	// I Initialized the perspective camera state.
	perspectivePosition = g_pCamera->Position;
	perspectiveFront = g_pCamera->Front;
	perspectiveUp = g_pCamera->Up;
	perspectiveYaw = g_pCamera->Yaw;
	perspectivePitch = g_pCamera->Pitch;

	// I Initialized the orthographic camera state (top-down view).
	orthographicPosition = glm::vec3(0.0f, 15.0f, 0.0f);
	orthographicFront = glm::vec3(0.0f, -1.0f, 0.0f);
	orthographicUp = glm::vec3(0.0f, 0.0f, -1.0f);
	orthographicYaw = -90.0f;
	orthographicPitch = -89.0f;

	cameraStatesInitialized = true;

}

/***********************************************************
 *  ~ViewManager()
 *
 *  The destructor for the class
 ***********************************************************/
ViewManager::~ViewManager()
{
	// free up allocated memory
	m_pShaderManager = NULL;
	m_pWindow = NULL;
	if (NULL != g_pCamera)
	{
		delete g_pCamera;
		g_pCamera = NULL;
	}
}

/***********************************************************
 *  CreateDisplayWindow()
 *
 *  This method is used to create the main display window.
 ***********************************************************/
GLFWwindow* ViewManager::CreateDisplayWindow(const char* windowTitle)
{
	GLFWwindow* window = nullptr;

	// try to create the displayed OpenGL window
	window = glfwCreateWindow(
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		windowTitle,
		NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return NULL;
	}
	glfwMakeContextCurrent(window);

	// tell GLFW to capture all mouse events
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);//I uncommented this to enable the cursor for camera control.

	// this callback is used to receive mouse moving events
	glfwSetCursorPosCallback(window, &ViewManager::Mouse_Position_Callback);

	// I added this line to register the scroll callback:
	glfwSetScrollCallback(window, &ViewManager::Mouse_Scroll_Callback);

	// enable blending for supporting tranparent rendering
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_pWindow = window;

	return(window);
}

/***********************************************************
 *  Mouse_Position_Callback()
 *
 *  This method is automatically called from GLFW whenever
 *  the mouse is moved within the active GLFW display window.
 ***********************************************************/
void ViewManager::Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos)
{
	// I added this code to handle first mouse movement to prevent camera jump.
	if (gFirstMouse)
	{
		gLastX = xMousePos;
		gLastY = yMousePos;
		gFirstMouse = false;
	}

	// I added this code to calculate the mouse offset from last the position.
	float xOffset = xMousePos - gLastX;
	float yOffset = gLastY - yMousePos; // I added this code to reverse since y-coordinates go from bottom to top.

	// I added this code to update the last mouse position.
	gLastX = xMousePos;
	gLastY = yMousePos;

	// I added this code to process the mouse movement for the camera orientation.
	if (g_pCamera)
	{
		g_pCamera->ProcessMouseMovement(xOffset, yOffset);
	}
}
/***********************************************************
 *  Mouse_Scroll_Callback()
 *	//I added this section for the Mouse Scroll speed and
 *	movement.
 *  This method is automatically called from GLFW whenever
 *  the mouse scroll wheel is used. It adjusts the camera
 *  movement speed, allowing users to control how fast they
 *  navigate through the scene.
 ***********************************************************/
void ViewManager::Mouse_Scroll_Callback(GLFWwindow* window, double xOffset, double yOffset)
{
	//I adjusted the camera speed based on the scroll direction.
	// Positive yOffset = scroll up = increase speed.
	// Negative yOffset = scroll down = decrease speed.
	gCameraSpeed += static_cast<float>(yOffset) * 0.5f;

	//I clamped the camera speed to stay within reasonable bounds.
	if (gCameraSpeed < MIN_CAMERA_SPEED)
	{
		gCameraSpeed = MIN_CAMERA_SPEED;
	}
	else if (gCameraSpeed > MAX_CAMERA_SPEED)
	{
		gCameraSpeed = MAX_CAMERA_SPEED;
	}

	//This updates the camera's movement speed.
	if (g_pCamera != nullptr)
	{
		g_pCamera->MovementSpeed = gCameraSpeed;
	}
}

/***********************************************************
 *  SwitchToOrthographic()
 *
 *  Switches camera to orthographic mode with appropriate
 *  settings for top-down view.
 ***********************************************************/
void ViewManager::SwitchToOrthographic()
{
	if (!bOrthographicProjection && cameraStatesInitialized)
	{
		// I adde this to Save the current perspective camera state.
		perspectivePosition = g_pCamera->Position;
		perspectiveFront = g_pCamera->Front;
		perspectiveUp = g_pCamera->Up;
		perspectiveYaw = g_pCamera->Yaw;
		perspectivePitch = g_pCamera->Pitch;

		// This Loads the orthographic camera state.
		g_pCamera->Position = orthographicPosition;
		g_pCamera->Front = orthographicFront;
		g_pCamera->Up = orthographicUp;
		g_pCamera->Yaw = orthographicYaw;
		g_pCamera->Pitch = orthographicPitch;

		// This triggers the camera vector update by using ProcessMouseMovement with zero offset.
		g_pCamera->ProcessMouseMovement(0.0f, 0.0f);

		bOrthographicProjection = true;
	}
}

/***********************************************************
 *  SwitchToPerspective()
 *
 *  Switches camera to perspective mode with appropriate
 *  settings.
 ***********************************************************/
void ViewManager::SwitchToPerspective()
{
	if (bOrthographicProjection && cameraStatesInitialized)
	{
		// I added this to Save the current orthographic camera state.
		orthographicPosition = g_pCamera->Position;
		orthographicFront = g_pCamera->Front;
		orthographicUp = g_pCamera->Up;
		orthographicYaw = g_pCamera->Yaw;
		orthographicPitch = g_pCamera->Pitch;

		// This Loads the perspective camera state.
		g_pCamera->Position = perspectivePosition;
		g_pCamera->Front = perspectiveFront;
		g_pCamera->Up = perspectiveUp;
		g_pCamera->Yaw = perspectiveYaw;
		g_pCamera->Pitch = perspectivePitch;

		// This triggers the camera vector update by using ProcessMouseMovement with zero offset.
		g_pCamera->ProcessMouseMovement(0.0f, 0.0f);

		bOrthographicProjection = false;
	}
}

/***********************************************************
 *  ProcessKeyboardEvents()
 *
 *  This method is called to process any keyboard events
 *  that may be waiting in the event queue.
 ***********************************************************/
void ViewManager::ProcessKeyboardEvents()
{
	// close the window if the escape key has been pressed
	if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(m_pWindow, true);
	}
	// I added this code to process the camera movement with WASD keys.
	if (glfwGetKey(m_pWindow, GLFW_KEY_W) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(FORWARD, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_S) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(BACKWARD, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_A) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(LEFT, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_D) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(RIGHT, gDeltaTime);
	}

	// I added this code to process the camera vertical movement with the QE keys.
	if (glfwGetKey(m_pWindow, GLFW_KEY_Q) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(UP, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_E) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(DOWN, gDeltaTime);
	}

	// I added this code to toggle the perspective projection with the P key.
	static bool pKeyWasPressed = false;
	bool pKeyIsPressed = glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_PRESS;
	if (pKeyIsPressed && !pKeyWasPressed)
	{
		SwitchToPerspective();
	}
	pKeyWasPressed = pKeyIsPressed;

	// I added this code to toggle the orthographic projection with the O key.
	static bool oKeyWasPressed = false;
	bool oKeyIsPressed = glfwGetKey(m_pWindow, GLFW_KEY_O) == GLFW_PRESS;
	if (oKeyIsPressed && !oKeyWasPressed)
	{
		SwitchToOrthographic();
	}
	oKeyWasPressed = oKeyIsPressed;
}

/***********************************************************
 *  PrepareSceneView()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void ViewManager::PrepareSceneView()
{
	glm::mat4 view;
	glm::mat4 projection;

	// per-frame timing
	float currentFrame = glfwGetTime();
	gDeltaTime = currentFrame - gLastFrame;
	gLastFrame = currentFrame;

	// process any keyboard events that may be waiting in the 
	// event queue
	ProcessKeyboardEvents();

	// get the current view matrix from the camera
	view = g_pCamera->GetViewMatrix();

	// I added this code to create the projection matrix based on current mode.
	if (bOrthographicProjection)
	{
		// The orthographic projection in 2D view.
		// I defined the orthographic view volume.
		float orthoScale = 10.0f;
		projection = glm::ortho(         
			-orthoScale * ((float)WINDOW_WIDTH / (float)WINDOW_HEIGHT), // The left
			orthoScale * ((float)WINDOW_WIDTH / (float)WINDOW_HEIGHT),  // The right.
			-orthoScale,  // The bottom.
			orthoScale,   // The top.
			0.1f,         // Near the plane.
			100.0f        // The far plane.
		);
	}
	else
	{
		// The perspective projection in 3D view.
		projection = glm::perspective(
			glm::radians(g_pCamera->Zoom),
			(GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT,
			0.1f,
			100.0f
		);
	}

	// if the shader manager object is valid
	if (NULL != m_pShaderManager)
	{
		// set the view matrix into the shader for proper rendering
		m_pShaderManager->setMat4Value(g_ViewName, view);
		// set the view matrix into the shader for proper rendering
		m_pShaderManager->setMat4Value(g_ProjectionName, projection);
		// set the view position of the camera into the shader for proper rendering
		m_pShaderManager->setVec3Value("viewPosition", g_pCamera->Position);
	}
	//Ben Douglas- I added the W key for up, the S key for down, the A key for left, and the D key for right.
	// I added the Q key to look up, and the E key to look down.
	// I added the P key for a perspective look, and the O key for a orthographic look.
	// I added the mouse scroll back to move fast or slow down around the scene.
	// I clamped the camera speed so that it doesn't go too fast.
	// I added code to get the last known location of the mouse position.
	//11-21-2025.
	//Ben Douglas- I added the perspective and orthographic switching.
	// I fixed the orthographic projection.
	//I changed the camera settings so that you can switch between the perspective and orthographic views.
	//11-28-2025.
	//Ben Douglas- I changed the camera.h file back to the original code.
	// I added g_pCamera->ProcessMouseMovement(0.0f, 0.0f); to the perspective and orthographic
	// to trigger the camera vector update by using the ProcessMouseMovement with zero offset.
	//12-04-2025
}