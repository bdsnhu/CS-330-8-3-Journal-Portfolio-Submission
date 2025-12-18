///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

void SceneManager::LoadSceneTextures()
{

	bool bReturn = false;

	// This loads the grass texture for the main ground plane.
	bReturn = CreateGLTexture("textures/plants_grass_seamless.jpg", "grass");

	// This loads the dirt/soil texture for the brown ground patch.
	bReturn = CreateGLTexture("textures/dirt.jpg", "dirt");

	// This loads the brick texture for the decorative path.
	bReturn = CreateGLTexture("textures/brick.jpg", "brick");

	// This loads the hedge/foliage texture for the rectangular hedge bush.
	bReturn = CreateGLTexture("textures/plants_hedge_seamless.jpg", "hedge");

	// This loads a second foliage texture for the pyramid bush (variation adds realism).
	bReturn = CreateGLTexture("textures/foliage.jpg", "foliage");

	// This binds all of the loaded textures to their respective slots.
	BindGLTextures();
}

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
	/*** There is no limit to the number of object materials that can ***/
	/*** be defined. Refer to the code in the OpenGL Sample for help ***/

	// Material for grass plane.
	OBJECT_MATERIAL grassMaterial;
	grassMaterial.ambientColor = glm::vec3(0.4f, 0.6f, 0.3f);
	grassMaterial.ambientStrength = 0.03f; // Extremely low
	grassMaterial.diffuseColor = glm::vec3(0.4f, 0.6f, 0.3f);
	grassMaterial.specularColor = glm::vec3(0.35f, 0.45f, 0.35f);
	grassMaterial.shininess = 5.0;
	grassMaterial.tag = "grass";
	m_objectMaterials.push_back(grassMaterial);

	// Material for dirt/soil - the darkest shadows possible.
	OBJECT_MATERIAL dirtMaterial;
	dirtMaterial.ambientColor = glm::vec3(0.5f, 0.4f, 0.3f);
	dirtMaterial.ambientStrength = 0.01f; // This is rock bottom for the near-black shadows.
	dirtMaterial.diffuseColor = glm::vec3(0.5f, 0.4f, 0.3f);
	dirtMaterial.specularColor = glm::vec3(0.18f, 0.18f, 0.18f);
	dirtMaterial.shininess = 1.2;
	dirtMaterial.tag = "dirt";
	m_objectMaterials.push_back(dirtMaterial);

	// Material for brick.
	OBJECT_MATERIAL brickMaterial;
	brickMaterial.ambientColor = glm::vec3(0.6f, 0.4f, 0.3f);
	brickMaterial.ambientStrength = 0.05f; // This is very low.
	brickMaterial.diffuseColor = glm::vec3(0.6f, 0.4f, 0.3f);
	brickMaterial.specularColor = glm::vec3(0.45f, 0.35f, 0.35f);
	brickMaterial.shininess = 4.0;
	brickMaterial.tag = "brick";
	m_objectMaterials.push_back(brickMaterial);

	// Material for the hedge foliage.
	OBJECT_MATERIAL hedgeMaterial;
	hedgeMaterial.ambientColor = glm::vec3(0.3f, 0.5f, 0.2f);
	hedgeMaterial.ambientStrength = 0.06f; // This is low.
	hedgeMaterial.diffuseColor = glm::vec3(0.3f, 0.5f, 0.2f);
	hedgeMaterial.specularColor = glm::vec3(0.22f, 0.32f, 0.22f);
	hedgeMaterial.shininess = 3.0;
	hedgeMaterial.tag = "hedge";
	m_objectMaterials.push_back(hedgeMaterial);

	// Material for the pyramid foliage.
	OBJECT_MATERIAL foliageMaterial;
	foliageMaterial.ambientColor = glm::vec3(0.35f, 0.55f, 0.25f);
	foliageMaterial.ambientStrength = 0.06f; // This is low.
	foliageMaterial.diffuseColor = glm::vec3(0.35f, 0.55f, 0.25f);
	foliageMaterial.specularColor = glm::vec3(0.28f, 0.35f, 0.28f);
	foliageMaterial.shininess = 7.0; // This is high for the brilliant highlights.
	foliageMaterial.tag = "foliage";
	m_objectMaterials.push_back(foliageMaterial);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene. There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// This line of code is NEEDED for telling the shaders to render
	// the 3D scene with custom lighting. If no light sources have
	// been added then the display window will be black.
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	/*** STUDENTS - add the code BELOW for setting up light sources ***/

	// This is more dramatic directional light.
	m_pShaderManager->setVec3Value("directionalLight.direction", -0.5f, -1.0f, -0.3f);
	m_pShaderManager->setVec3Value("directionalLight.ambient", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 1.5f, 1.5f, 1.4f);  // I increased this.
	m_pShaderManager->setVec3Value("directionalLight.specular", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	// This is the fill light.
	m_pShaderManager->setVec3Value("pointLights[0].position", 3.5f, 5.0f, 1.5f);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.4f, 0.4f, 0.35f);
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

	// This is a warm-colored fill light for the left side.
	m_pShaderManager->setVec3Value("pointLights[1].position", -3.5f, 5.0f, 6.5f);
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.15f, 0.1f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.8f, 0.6f, 0.3f);  // This is a Warm orange/amber color.
	m_pShaderManager->setVec3Value("pointLights[1].specular", 0.4f, 0.3f, 0.2f);
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene.
	//This loads all the meshes that will be used in the scene.
	m_basicMeshes->LoadPlaneMesh();//This loads in the plane.
	m_basicMeshes->LoadBoxMesh();//I added this to load the boxes to create a rectangle. 
	m_basicMeshes->LoadPyramid4Mesh();//I added this to load the pyramid to make the pyramid bush. 
	//I added the LoadPyramid4Mesh to go from a 3-sided pyramid to a 4-sided pyramid.
	m_basicMeshes->LoadConeMesh(); // I added this line to load the cone mesh in.

	// This loads all of the textures for the scene.
	LoadSceneTextures();
	DefineObjectMaterials();// This loads all of the materials for the scene.
	SetupSceneLights();// This loads all of the lights for the scene.
	
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/

/******************************************************************/
/* THIS IS THE MAIN GRASS GROUND PLANE WITH TILED TEXTURE AND     
LIGHTING.                                                         */
/******************************************************************/
	
/******************************************************************/
/* I RENDERED THE MAIN GRASS GROUND PLANE WITH TILED TEXTURE.     */
/******************************************************************/

	// I set the scale for the main ground plane.
	scaleXYZ = glm::vec3(20.0f, 1.0f, 15.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// I applied transformations.
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	// I set the UV scale to tile the grass texture across the large plane.
	// This creates a realistic tiled grass effect (COMPLEX TEXTURING TECHNIQUE - TILING).
	SetTextureUVScale(4.0f, 2.0f);

	// I applied the material properties for the grass with lighting.
	SetShaderMaterial("grass");

	// I applied the grass texture.
	SetShaderTexture("grass");

	// This draws the textured ground plane.
	m_basicMeshes->DrawPlaneMesh();

/******************************************************************/
/* THIS IS THE BROWN/TAN DIRT PATCH WITH TEXTURE AND LIGHTING.    */
/******************************************************************/

/******************************************************************/
/* I RENDERED THE BROWN/TAN DIRT PATCH WITH TEXTURE.              */
/******************************************************************/

	// I set the scale for the dirt patch under the topiary.
	scaleXYZ = glm::vec3(8.0f, 3.5f, 8.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// I positioned the dirt slightly above grass to prevent z-fighting.
	positionXYZ = glm::vec3(0.0f, 0.02f, 6.5f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	// I set the UV scale for dirt texture tiling.
	SetTextureUVScale(2.0f, 2.0f);

	// I applied the material properties for dirt with lighting.
	SetShaderMaterial("dirt");

	// I applied the dirt texture.
	SetShaderTexture("dirt");

	// This draws the dirt patch.
	m_basicMeshes->DrawPlaneMesh();

/******************************************************************/
/* THIS IS THE BRICK PATH WITH TEXTURE AND LIGHTING 
(45 DEGREE ANGLE).                                                */
/******************************************************************/

/******************************************************************/
/* I RENDERED THE BRICK PATH WITH TEXTURE AT A (45 DEGREE ANGLE). */
/******************************************************************/

	// This does the brick dimensions and rotation.
	scaleXYZ = glm::vec3(0.5f, 0.15f, 0.5f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 45.0f;// I rotated to match the topiary orientation.
	ZrotationDegrees = 0.0f;

	// I set the UV scale for individual brick texture mapping.
	SetTextureUVScale(1.0f, 1.0f);

	// I applied the material properties for bricks with lighting.
	SetShaderMaterial("brick");

	// I applied the brick texture that (will be used for all bricks).
	SetShaderTexture("brick");

	// THIS IS BRICK PATH - ROW 1.
	// This is brick 1.
	positionXYZ = glm::vec3(-1.2f, 0.08f, 7.2f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_basicMeshes->DrawBoxMesh();

	// This is brick 2.
	positionXYZ = glm::vec3(-1.6f, 0.08f, 7.6f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_basicMeshes->DrawBoxMesh();

	// This is brick 3.
	positionXYZ = glm::vec3(-2.0f, 0.08f, 8.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_basicMeshes->DrawBoxMesh();

	// This is brick 4.
	positionXYZ = glm::vec3(-2.4f, 0.08f, 8.4f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_basicMeshes->DrawBoxMesh();

	// This is brick 5.
	positionXYZ = glm::vec3(-2.8f, 0.08f, 8.8f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_basicMeshes->DrawBoxMesh();

	// THIS IS BRICK PATH - ROW 2.
	// This is brick 1.
	positionXYZ = glm::vec3(-0.8f, 0.08f, 7.6f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_basicMeshes->DrawBoxMesh();

	// This is brick 2.
	positionXYZ = glm::vec3(-1.2f, 0.08f, 8.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_basicMeshes->DrawBoxMesh();

	// This is brick 3.
	positionXYZ = glm::vec3(-1.6f, 0.08f, 8.4f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_basicMeshes->DrawBoxMesh();

	// This is brick 4.
	positionXYZ = glm::vec3(-2.0f, 0.08f, 8.8f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_basicMeshes->DrawBoxMesh();

	// This is brick 5.
	positionXYZ = glm::vec3(-2.4f, 0.08f, 9.2f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_basicMeshes->DrawBoxMesh();
	
/******************************************************************/
/* I RENDERED THE COMPLEX TOPIARY OBJECT OF THE 
   (RECTANGULAR + PYRAMID).                                       */
/* I demonstrated COHESIVE OBJECT with DIFFERENT TEXTURES.        */
/******************************************************************/

	// This is the rectangular hedge bush or the (Bottom component).
	scaleXYZ = glm::vec3(2.0f, 1.0f, 1.5f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 45.0f;// I rotated to match scene orientation.
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 0.75f, 6.5f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	// I set the UV scale for the hedge texture.
	SetTextureUVScale(2.0f, 1.0f);

	// I applied the material properties for hedge with lighting.
	SetShaderMaterial("hedge");

	// I applied the hedge texture to the rectangular bush.
	SetShaderTexture("hedge");

	// This draws the rectangular hedge component.
	m_basicMeshes->DrawBoxMesh();

	// This is the pyramid bush or the (Top component).
	// This creates a cohesive topiary by using a different but complementary texture.
	scaleXYZ = glm::vec3(1.5f, 2.5f, 1.5f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 45.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 2.5f, 6.5f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	// I set the UV scale for the foliage texture.
	SetTextureUVScale(1.5f, 1.5f);

	// I applied the material properties for foliage with lighting.
	SetShaderMaterial("foliage");

	// I applied the different foliage texture to the pyramid to (create a visual interest and realism).
	SetShaderTexture("foliage");

	// This draws the pyramid bush component.
	m_basicMeshes->DrawPyramid4Mesh();

/******************************************************************/
/* THIS IS AN ADDITIONAL TOPIARY 1 - FIRST IN LINE NEXT TO 
PYRAMID.															  */
/******************************************************************/

	// This loads the cone mesh once.
	m_basicMeshes->LoadConeMesh();

	// This is the rectangle base for topiary 1.
	scaleXYZ = glm::vec3(2.0f, 1.0f, 1.5f);// I matched to the main topiary rectangle dimensions.
	XrotationDegrees = 0.0f;
	YrotationDegrees = 45.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(1.5f, 0.75f, 5.0f);// I adjusted to move much closer to the main topiary.

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetTextureUVScale(1.5f, 1.0f);
	SetShaderMaterial("hedge");
	SetShaderTexture("hedge");
	m_basicMeshes->DrawBoxMesh();

	// This is the cone top for topiary 1.
	scaleXYZ = glm::vec3(0.7f, 1.0f, 0.7f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 45.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(1.5f, 1.25f, 5.0f);// I lowered to sit it closer to the rectangle.

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetTextureUVScale(1.2f, 1.2f);
	SetShaderMaterial("foliage");
	SetShaderTexture("foliage");
	m_basicMeshes->DrawConeMesh();

/******************************************************************/
/* THIS IS THE ADDITIONAL TOPIARY 2 - SECOND IN LINE.             */
/******************************************************************/

	// This is the rectangle base for topiary 2.
	scaleXYZ = glm::vec3(2.0f, 1.0f, 1.5f);// I matched to the main topiary rectangle dimensions.
	XrotationDegrees = 0.0f;
	YrotationDegrees = 45.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(3.0f, 0.75f, 3.5f);  // I made much tighter spacing to look like the image.

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetTextureUVScale(1.5f, 1.0f);
	SetShaderMaterial("hedge");
	SetShaderTexture("hedge");
	m_basicMeshes->DrawBoxMesh();

	// This is the cone top for topiary 2.
	scaleXYZ = glm::vec3(0.75f, 1.0f, 0.75f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 45.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(3.0f, 1.25f, 3.5f);  // I lowered to sit closer to the rectangle.

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetTextureUVScale(1.2f, 1.2f);
	SetShaderMaterial("foliage");
	SetShaderTexture("foliage");
	m_basicMeshes->DrawConeMesh();

/******************************************************************/
/* THIS IS AN ADDITIONAL TOPIARY 3 - THIRD IN LINE.               */
/******************************************************************/

	// This is the rectangle base for topiary 3.
	scaleXYZ = glm::vec3(2.0f, 1.0f, 1.5f);  // I matched to the main topiary rectangle dimensions.
	XrotationDegrees = 0.0f;
	YrotationDegrees = 45.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(4.5f, 0.75f, 2.0f);  // I made much tighter spacing to look like the image.

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetTextureUVScale(1.5f, 1.0f);
	SetShaderMaterial("hedge");
	SetShaderTexture("hedge");
	m_basicMeshes->DrawBoxMesh();

	// Cone top for topiary 3.
	scaleXYZ = glm::vec3(0.65f, 1.0f, 0.65f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 45.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(4.5f, 1.25f, 2.0f);  // I lowered the cone to sit closer on the rectangle.

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetTextureUVScale(1.2f, 1.2f);
	SetShaderMaterial("foliage");
	SetShaderTexture("foliage");
	m_basicMeshes->DrawConeMesh();

	// Ben Douglas- I changed the color of the plane from white to green to match the green grass in the topiary bushes picture.
	// I added the box mesh and combine the boxes to make a recantangle to represent the rectangle hedge bush in the topiary bushes picture.
	// I used darker green to color the rectangle hedge bush to differentiate among the plane grass and the pyramid bush, and to replicate the picture.
	// I adjusted the numbers to get the scale of the rectangle to be like the rectangle bush in the picture.
	// I adjusted the numbers to postioned the rectangle hedge bush under the pyramid bush.
	// I added the pyramid mesh to represent the pyramid bush in the topiary bushes picture.
	// I used medium green to color the pyramid bush to differentiate among the plane grass and the rectangle hedge bush in the picture.
	// I adjusted the numbers to get the scale of the pyramid to be like the pyramid bush in the picture.
	// I adjusted the numbers to positioned the pyramid bush on top of the rectangle bush.
	// I constructed the 3D objects from combines boxes to make a rectangle, I used a pyramid mesh, and I combined the pyramid with the rectangle to replicate the 2D picture.
	// I added the LoadPyramid4Mesh, and the DrawPyramid4Mesh to go from a 3-sided pyramid to a 4-sided pyramid.
	// 11-16-2025.
	// Ben Douglas- I added the LoadPyramid4Mesh, and the DrawPyramid4Mesh to go from a 3-sided pyramid to a 4-sided pyramid.
	// I added the brown/tan atop of the green plane to match the picture of the green grass, the tan bricks to the left of the pyramid bush,
	// and the brown/tan ground under the rectangular and pyramid bush.
	// I scaled the brown/tan plane to be big enough to fit the bushes on it.
	// I scaled the brown/tan bricks to be the right size next to the pyramid bush.
	// I set the color for the brown/tan plane to brown/tan.
	// I set the color for the bricks to be brown/tan.
	// I rotated the bricks to get them in the right postion.
	// 11-21-2025.
	// Ben Douglas- I applied detailed textures to grass plane, the dirt under the pyramid hedge bush, the rectangular hedge bush, the pyramid hedge bush, and the bricks
	// by using SetShaderTexture().
	// I deleted the colors for the scene to incoporate the images for the objects.
	// I used the textures of plants_grass_seamless.jpg, dirt.jpg, plants_hedge_seamless.jpg, foliage.jpg, and brick.jpg to render on the objects in the scene.
	// I used a complex technique called texture tiling by using SetTextureUVScale() on the grass and the dirt planes for realistic repetition.
	// I created a cohesive object by using the hedge texture on the rectangular bush and by using the foliage on the pyramid bush that looks like a unified look.
	// I created code quality to make sure the scene runs smoothly.
	// I used the best practice through modular texture of loading in the LoadSceneTextures(), consistent naming conventions, and proper texture binding.
	// 11-29-2025.
	// Ben Douglas- I created the void SceneManager::DefineObjectMaterials() for the scene.
	// I created the void SceneManager::SetupSceneLights() for the lighting of the scene.
	// I created the THIS IS THE MAIN GRASS GROUND PLANE WITH TILED TEXTURE AND LIGHTING as well as the other materials in the scene for the shaders.
	// 12-06-2025.
	// Ben Douglas- I added a third light that looks like the color of orange to get rid of the shadow on the left side.
	// I added m_basicMeshes->LoadConeMesh(); to load the cone mesh to make the cone bushes.
	// I created three rectangle bushes to look like the image.
	// I created three cone bushes and put them on top of the rectangle bushes.
	// Overall I added the green grass, the soil, the pyramid bush, the bricks, and the 3 cone bushes to make it look like a topiary garden image.
	// 12-12-2025.
	/****************************************************************/
}
