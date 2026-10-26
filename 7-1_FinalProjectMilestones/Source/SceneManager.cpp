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

/***********************************************************
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;
	// black plastic texture from google
	bReturn = CreateGLTexture(
		"../../Utilities/textures/black-plastic.jpg",
		"bplastic");

	// white shelf plaster texture from google
	bReturn = CreateGLTexture(
		"../../Utilities/textures/shelf.jpg",
		"shelf");

	// gray gradient texture for inside lightstick
	bReturn = CreateGLTexture(
		"../../Utilities/textures/gray-gradient.jpg",
		"gradient");

	// fur texture for plush
	bReturn = CreateGLTexture("../../Utilities/textures/fur.jpg",
		"fur");

	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
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

	// Gold Material from pdf
	OBJECT_MATERIAL goldMaterial;
	goldMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.1f);
	goldMaterial.ambientStrength = 0.4f;
	goldMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.2f);
	goldMaterial.specularColor = glm::vec3(0.6f, 0.5f, 0.4f);
	goldMaterial.shininess = 22.0;
	goldMaterial.tag = "gold";
	m_objectMaterials.push_back(goldMaterial);

	// Cement Material from pdf
	OBJECT_MATERIAL cementMaterial;
	cementMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	cementMaterial.ambientStrength = 0.2f;
	cementMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	cementMaterial.specularColor = glm::vec3(0.4f, 0.4f, 0.4f);
	cementMaterial.shininess = 0.5;
	cementMaterial.tag = "cement";
	m_objectMaterials.push_back(cementMaterial);

	// Wood Material (altered from pdf)
	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.3f, 0.3f, 0.3f);
	woodMaterial.ambientStrength = 0.1f;
	woodMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.shininess = 0.3;
	woodMaterial.tag = "wood";
	m_objectMaterials.push_back(woodMaterial);

	// Glossy Plastic Material 
	OBJECT_MATERIAL plasticMaterial;
	plasticMaterial.ambientColor = glm::vec3(0.25f, 0.25f, 0.25f);
	plasticMaterial.ambientStrength = 0.1f;
	plasticMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);   // low
	plasticMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);  // bright white highlight
	plasticMaterial.shininess = 15.0;                             
	plasticMaterial.tag = "plastic";
	m_objectMaterials.push_back(plasticMaterial);

	// Tile Material from pdf
	OBJECT_MATERIAL tileMaterial;
	tileMaterial.ambientColor = glm::vec3(0.2f, 0.3f, 0.4f);
	tileMaterial.ambientStrength = 0.3f;
	tileMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.1f);
	tileMaterial.specularColor = glm::vec3(0.4f, 0.5f, 0.6f);
	tileMaterial.shininess = 25.0;
	tileMaterial.tag = "tile";
	m_objectMaterials.push_back(tileMaterial);

	// Glass Material from pdf
	OBJECT_MATERIAL glassMaterial;
	glassMaterial.ambientColor = glm::vec3(0.4f, 0.4f, 0.4f);
	glassMaterial.ambientStrength = 0.3f;
	glassMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	glassMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f);
	glassMaterial.shininess = 85.0;
	glassMaterial.tag = "glass";
	m_objectMaterials.push_back(glassMaterial);

	// Clay Material from pdf
	OBJECT_MATERIAL clayMaterial;
	clayMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.3f);
	clayMaterial.ambientStrength = 0.3f;
	clayMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.5f);
	clayMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.4f);
	clayMaterial.shininess = 0.5;
	clayMaterial.tag = "clay";
	m_objectMaterials.push_back(clayMaterial);

}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// Help guide for my adjusting
	// position - right, up, forward
	// ambient color - faint base glow that hits every surface, even those turned away from light
	// diffuse color - the main color of the light on surfaces facing it
	// specular color - highlight spot color
	// focal strength - how tight the highlight is
	// specular intensity - highlight brightness multiplier

	// Light 0: Main light | color: white | position: high overhead, slightly to the right
	m_pShaderManager->setVec3Value("lightSources[0].position", 3.0f, 14.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.03f, 0.35f, 0.03f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.35f, 0.35f, 0.35f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 30.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.025f);

	// Light 1: Fill light | color: white | position: main light mirrored on the left | dimmer intensity
	m_pShaderManager->setVec3Value("lightSources[1].position", -3.0f, 14.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.0f, 0.0f, 0.2f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.12f, 0.12f, 0.12f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 20.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.01f);

	// Light 2: Accent light | color: white | position: in front of objects
	m_pShaderManager->setVec3Value("lightSources[2].position", -2.0f, 10.0f, 15.0f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.02f, 0.05f, 0.2f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.15f, 0.15f, 0.15f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.4f, 0.4f, 0.4f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 12.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.05f);

	// Light 3: Lightstick light | color: blue | highest intensity
	m_pShaderManager->setVec3Value("lightSources[3].position", 0.0f, 7.5f, 1.0f); // coming from the center of the bulb
	m_pShaderManager->setVec3Value("lightSources[3].ambientColor", 0.0f, 0.0f, 0.1f);
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", 0.05f, 0.15f, 0.5f);
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", 0.05f, 0.15f, 0.5f);
	m_pShaderManager->setVec3Value("lightSources[3].specularColor", 0.1f, 0.3f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 12.0f);
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 0.2f);

	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting, if no light sources have
	// been added then the display window will be black - to use the 
	// default OpenGL lighting then comment out the following line
	m_pShaderManager->setBoolValue("bUseLighting", true);
}
/***********************************************************
 *  OffsetAroundY()
 *
 *  helper function for my object functions that takes the base
 *  position, and an offset to rotate around the y axis
 *  This keeps all parts of an object rotated together as one object,
 *  made it a function instead of writing this over and over.
 ***********************************************************/
static glm::vec3 OffsetAroundY(float x, float y, float z,float ox, float oy, float oz, float rotYDeg)
{
	float r = glm::radians(rotYDeg);
	return glm::vec3(x + ox * cosf(r) + oz * sinf(r), y + oy, z - ox * sinf(r) + oz * cosf(r));
}
/**************************************************************/
/*** OBJECT FUNCTIONS									    ***/
/***														***/
/*** DrawWalls(), DrawLightstick(), DrawNotebook(),		    ***/
/*** DrawCandle(), DrawCat(), DrawCamera()                  ***/
/**************************************************************/
/***********************************************************
 *  DrawLightstick()
 *
 *  draws the BTS lightstick 
 *  handle, connector, symbol stand, two prisms, glass bulb, and top piece
 *  x, y, z center of the bottom of the handle
 *  scalar s
 ***********************************************************/
void SceneManager::DrawLightstick(float x, float y, float z, float scale) {
	glm::vec3 scaleXYZ; // set scale
	glm::vec3 positionXYZ; // set position
	float s = scale; // Can resize w/ this scalar variable

	// Handle - cylinder
	scaleXYZ = glm::vec3(0.75f * s, 6.0f * s, 1.0f * s); // use original scale from earlier, allow it to scale w/ scalar
	positionXYZ = glm::vec3(x, y, z);
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // dark gray
	SetShaderTexture("bplastic"); // black plastic texture
	SetShaderMaterial("wood"); // less reflective
	m_basicMeshes->DrawCylinderMesh();

	// Handle to glass bulb connector - cylinder
	scaleXYZ = glm::vec3(1.25f * s, 0.5f * s, 1.0f * s); // use original scale from earlier, allow it to scale w/ scalar
	positionXYZ = glm::vec3(x, y + 5.5f * s, z); // use original position and allow it to change w/ scalar
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f);
	SetShaderTexture("bplastic");
	SetShaderMaterial("tile");
	m_basicMeshes->DrawCylinderMesh();

	// Symbol stand inside the glass bulb - tapered cylinder
	scaleXYZ = glm::vec3(1.5f * s, 0.75f * s, 1.0f * s); // use original scale from earlier, allow it to scale w/ scalar
	positionXYZ = glm::vec3(x, y + 6.0f * s, z); // use original position and allow it to change w/ scalar
	SetTransformations(scaleXYZ, 0, 0, -180.0f, positionXYZ); // z rotation
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f);
	SetShaderTexture("bplastic");
	SetShaderMaterial("tile");
	m_basicMeshes->DrawTaperedCylinderMesh();

	// Right prism symbol - prism
	scaleXYZ = glm::vec3(0.75f * s, 2.5f * s, 1.5f * s); // use original scale from earlier, allow it to scale w/ scalar use this w/ other prism
	positionXYZ = glm::vec3(x + 0.5f * s, y + 7.25f * s, z); // use original position and allow it to change w/ scalar. 
	SetTransformations(scaleXYZ, 0, 0, -180.0f, positionXYZ); // z rotation
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f);
	SetShaderTexture("gradient"); // gradient texture to differentiate symbol
	m_basicMeshes->DrawPrismMesh();

	// Left prism symbol - prism
	positionXYZ = glm::vec3(x - 0.5f * s, y + 7.25f * s, z); // use original position and allow it to change w/ scalar. 
	SetTransformations(scaleXYZ, 0, 0, -180.0f, positionXYZ); // z rotation
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f);
	SetShaderTexture("gradient"); // gradient texture to differentiate symbol
	m_basicMeshes->DrawPrismMesh();

	// Glass transparent bulb - sphere
	scaleXYZ = glm::vec3(2.0f * s, 1.75f * s, 2.5f * s); // use original scale from earlier, allow it to scale w/ scalar
	positionXYZ = glm::vec3(x, y + 7.5f * s, z); // use original position and allow it to change w/ scalar. 
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 0.3f); // clear but still defined
	SetShaderMaterial("glass"); // glass material
	m_basicMeshes->DrawSphereMesh();

	// Decoration on top of bulb - cylinder
	scaleXYZ = glm::vec3(0.75f * s, 0.15f * s, 1.0f * s); // use original scale from earlier, allow it to scale w/ scalar
	positionXYZ = glm::vec3(x + 1.0f * s, y + 9.0f * s, z); // use original position and allow it to change w/ scalar.
	SetTransformations(scaleXYZ, 0, 0, 155.75f, positionXYZ); // z rotation
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // dark gray
	SetShaderTexture("bplastic");
	SetShaderMaterial("tile");
	m_basicMeshes->DrawCylinderMesh();
}
 /***********************************************************
  *  DrawNotebook()
  *
  *  draws the white/offwhite notebook 
  *  bottom cover, pages, top cover, spine
  *  x, y, z center of the bottom of the notebook
  *  rotY rotates the whole notebook around the y axis
  *	 scalar s
  ***********************************************************/
void SceneManager::DrawNotebook(float x, float y, float z, float rotY, float scale) {
	glm::vec3 scaleXYZ; // set scale
	glm::vec3 positionXYZ; // set position
	float s = scale; // can resize w/ this scalar variable

	// Bottom cover - box
	scaleXYZ = glm::vec3(4.0f * s, 0.1f * s, 5.5f * s); // use original scale from earlier, allow it to scale w/ scalar
	positionXYZ = OffsetAroundY(x, y, z, 0.0f, 0.05f * s, 0.0f, rotY); // does the alignment math with whatever rotation angle I want for the object
	SetTransformations(scaleXYZ, 0, rotY, 0, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); // white
	SetShaderMaterial("plastic"); // plastic material
	m_basicMeshes->DrawBoxMesh();

	// Pages - box
	scaleXYZ = glm::vec3(3.7f * s, 0.4f * s, 5.2f * s); // use original scale from earlier, allow it to scale w/ scalar
	positionXYZ = OffsetAroundY(x, y, z, -0.1f * s, 0.30f * s, -0.1f * s, rotY); // does the alignment math with whatever rotation angle I want for the object
	SetTransformations(scaleXYZ, 0, rotY, 0, positionXYZ);
	SetShaderColor(0.98f, 0.97f, 0.93f, 1.0f); // slight off-white
	SetShaderMaterial("wood"); // wood material for less reflection
	m_basicMeshes->DrawBoxMesh();

	// Top cover - box
	scaleXYZ = glm::vec3(4.0f * s, 0.1f * s, 5.5f * s); // use original scale from earlier, allow it to scale w/ scalar
	positionXYZ = OffsetAroundY(x, y, z, 0.0f, 0.55f * s, 0.0f, rotY); // does the alignment math with whatever rotation angle I want for the object
	SetTransformations(scaleXYZ, 0, rotY, 0, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetShaderMaterial("plastic"); // plastic material
	m_basicMeshes->DrawBoxMesh();

	// Spine - box
	scaleXYZ = glm::vec3(0.2f * s, 0.62f * s, 5.5f * s); // use original scale from earlier, allow it to scale w/ scalar
	positionXYZ = OffsetAroundY(x, y, z, -1.9f * s, 0.30f * s, 0.0f, rotY); // does the alignment math with whatever rotation angle I want for the object
	SetTransformations(scaleXYZ, 0, rotY, 0, positionXYZ);
	SetShaderColor(0.85f, 0.82f, 0.75f, 1.0f); // more off-white than pages
	SetShaderMaterial("wood"); // wood material for less reflection 
	m_basicMeshes->DrawBoxMesh();
}
  /***********************************************************
   *  DrawCandle()
   *
   *  draws the candle 
   *  jar and lid
   *  x, y, z center of the jar base
   ***********************************************************/
void SceneManager::DrawCandle(float x, float y, float z, float scale) {
	glm::vec3 scaleXYZ; // set scale
	glm::vec3 positionXYZ; // set position

	// Reusable local variables
	float jarHeight = 3.4f * scale;
	float jarRadius = 2.5f * scale;
	float lidHeight = 0.35f * scale;

	// Jar - cylinder
	scaleXYZ = glm::vec3(jarRadius, jarHeight, jarRadius); // use scaled global values
	positionXYZ = glm::vec3(x, y, z);
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderColor(0.62f, 0.62f, 0.65f, 1.0f); // off white color
	SetShaderMaterial("glass"); // glass material
	m_basicMeshes->DrawCylinderMesh();

	// Lid - cylinder
	scaleXYZ = glm::vec3(jarRadius + 0.05f * scale, lidHeight, jarRadius + 0.05f * scale); // add additional scaling for lid definition
	positionXYZ = glm::vec3(x, y + jarHeight, z); // lid needs to sit on top of the jar
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderColor(0.80f, 0.68f, 0.38f, 1.0f); // gold color
	SetShaderMaterial("gold"); // gold material
	m_basicMeshes->DrawCylinderMesh();
}
 /***********************************************************
  *  DrawCat()
  *
  *  Draws the chococat character plush 
  *  body, two ears, insides of ears, eyes, pupils, nose
  *  x, y, z center of the cat
  *  scalar s
  ***********************************************************/
void SceneManager::DrawCat(float x, float y, float z, float scale)
{
	glm::vec3 scaleXYZ; // set scale
	glm::vec3 positionXYZ; // set position
	float s = scale; // scalar

	float bodyHalfHeight = 1.5f * s;              
	float bodyCenterY = y + bodyHalfHeight; // squashed sphere, I let it clip a little through the shelf to make it look like it had a flattish bottom
	float bodyTopY = y + bodyHalfHeight * 2.0f;

	// Body (head of cat) - sphere
	scaleXYZ = glm::vec3(2.6f * s, 1.8f * s, 1.5f * s); // scale my original settings
	positionXYZ = glm::vec3(x, bodyCenterY, z); // position the body differently than the other pieces
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f); // dark black
	SetShaderMaterial("clay");   // matte for the plush 
	SetShaderTexture("fur"); // furry texture
	m_basicMeshes->DrawSphereMesh();

	// Left ear - cone
	scaleXYZ = glm::vec3(0.75f * s, 1.5f * s, 0.4f * s);
	positionXYZ = glm::vec3(x - 1.8f * s, bodyTopY - 0.6f * s, z); // at top of the head
	SetTransformations(scaleXYZ, 0, 0, 20.0f, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f); // dark black
	SetShaderMaterial("clay"); // matte for plush
	SetShaderTexture("fur"); // furry texture
	m_basicMeshes->DrawConeMesh();

	// Left inner ear - tapered cylinder
	scaleXYZ = glm::vec3(0.4f * s, 0.75f * s, 0.2f * s); // dont make taller than the ear
	positionXYZ = glm::vec3(x - 1.85f * s, bodyTopY - 0.55f * s, z + 0.2f * s); // top of the head
	SetTransformations(scaleXYZ, 0, 0, 20.0f, positionXYZ);
	SetShaderColor(0.90f, 0.75f, 0.25f, 1.0f); // yellow
	SetShaderMaterial("clay"); // matte for plush
	m_basicMeshes->DrawTaperedCylinderMesh();

	// Right ear - cone
	scaleXYZ = glm::vec3(0.75f * s, 1.5f * s, 0.4f * s);
	positionXYZ = glm::vec3(x + 1.8f * s, bodyTopY - 0.6f * s, z); // at top of the head
	SetTransformations(scaleXYZ, 0, 0, -20.0f, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f); // dark black
	SetShaderMaterial("clay"); // matte for plush
	SetShaderTexture("fur"); // furry texture
	m_basicMeshes->DrawConeMesh();

	// Right inner ear - tapered cylinder
	scaleXYZ = glm::vec3(0.4f * s, 0.75f * s, 0.2f * s); // dont make taller than the ear
	positionXYZ = glm::vec3(x + 1.85f * s, bodyTopY - 0.55f * s, z + 0.2f * s); // top of the head
	SetTransformations(scaleXYZ, 0, 0, -20.0f, positionXYZ);
	SetShaderColor(0.90f, 0.75f, 0.25f, 1.0f); // yellow
	SetShaderMaterial("clay"); // matte for plush
	m_basicMeshes->DrawTaperedCylinderMesh();

	// Left eye - sphere
	scaleXYZ = glm::vec3(0.6f * s, 0.65f * s, 0.3f * s); // reuse scale for other eye
	positionXYZ = glm::vec3(x - 0.9f * s, bodyCenterY + 0.1f * s, z + 1.2f * s); // slight clip into the body
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetShaderMaterial("clay"); // matte for plush
	m_basicMeshes->DrawSphereMesh();

	// Right eye - sphere
	positionXYZ = glm::vec3(x + 0.9f * s, bodyCenterY + 0.1f * s, z + 1.2f * s);
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ); // slight clip into the body
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetShaderMaterial("clay"); // matte for plush
	m_basicMeshes->DrawSphereMesh();

	// Left pupil - sphere
	scaleXYZ = glm::vec3(0.25f * s, 0.28f * s, 0.12f * s); // reuse scale for other pupil
	positionXYZ = glm::vec3(x - 0.95f * s, bodyCenterY + 0.1f * s, z + 1.45f * s); // clip into the eye
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderTexture("fur"); // furry texture
	SetShaderMaterial("clay");
	m_basicMeshes->DrawSphereMesh();

	// Right pupil - sphere
	positionXYZ = glm::vec3(x + 0.85f * s, bodyCenterY + 0.1f * s, z + 1.45f * s);  // clip into the eye
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderMaterial("clay");
	SetShaderTexture("fur"); // furry texture
	m_basicMeshes->DrawSphereMesh();

	// Nose - sphere
	scaleXYZ = glm::vec3(0.2f * s, 0.15f * s, 0.15f * s);
	positionXYZ = glm::vec3(x, bodyCenterY - 0.5f * s, z + 1.4f * s);
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderColor(0.90f, 0.75f, 0.25f, 1.0f); // same as inner ears
	SetShaderMaterial("clay");
	m_basicMeshes->DrawSphereMesh();
}
  /***********************************************************
   *  DrawCamera()
   *
   *  draws canon camera 
   *  body, grip, top bump, shutter button, mode dial, logo, lens mount, lens barrel, lens
   *  x, y, z center of camera body.
   *  rotY turns the camera around y axis 
   *  scalar s
   ***********************************************************/
void SceneManager::DrawCamera(float x, float y, float z, float rotY, float scale)
{
	glm::vec3 scaleXYZ;
	glm::vec3 positionXYZ;
	float s = scale;
	float bodyMidY = 1.1f;   // local height of the center of the camera for other shape placements

	// Camera body - box
	scaleXYZ = glm::vec3(3.2f * s, 2.2f * s, 1.6f * s);
	positionXYZ = OffsetAroundY(x, y, z, 0.0f, bodyMidY * s, 0.0f, rotY); // rotated around the Y axis by rotY so pieces of camera turn as one
	SetTransformations(scaleXYZ, 0, rotY, 0, positionXYZ); 
	SetShaderTexture("bplastic"); // black plastic texture
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawBoxMesh();

	// Hand grip - box
	scaleXYZ = glm::vec3(0.6f * s, 2.0f * s, 0.3f * s);
	positionXYZ = OffsetAroundY(x, y, z, -1.3f * s, bodyMidY * s, 0.95f * s, rotY); // rotated around the Y axis by rotY so pieces of camera turn as one
	SetTransformations(scaleXYZ, 0, rotY, 0, positionXYZ);
	SetShaderMaterial("plastic");
	SetShaderTexture("gradient"); // stand out
	m_basicMeshes->DrawBoxMesh();

	// Top bump - box
	scaleXYZ = glm::vec3(1.1f * s, 0.5f * s, 1.0f * s); // rotated around the Y axis by rotY so pieces of camera turn as one
	positionXYZ = OffsetAroundY(x, y, z, 0.0f, 2.45f * s, -0.1f * s, rotY);
	SetTransformations(scaleXYZ, 0, rotY, 0, positionXYZ);
	SetShaderMaterial("plastic");
	SetShaderTexture("bplastic"); // black plastic texture
	m_basicMeshes->DrawBoxMesh();

	// Logo - box
	scaleXYZ = glm::vec3(0.6f * s, 0.1f * s, 0.02f * s);
	positionXYZ = OffsetAroundY(x, y, z, 0.0f, 2.45f * s, 0.41f * s, rotY); // rotated around the Y axis by rotY so pieces of camera turn as one
	SetTransformations(scaleXYZ, 0, rotY, 0, positionXYZ);
	SetShaderColor(0.9f, 0.9f, 0.9f, 1.0f);
	SetShaderMaterial("wood"); // not reflective
	m_basicMeshes->DrawBoxMesh();

	// Shutter button - cylinder
	scaleXYZ = glm::vec3(0.2f * s, 0.12f * s, 0.2f * s); // very small height
	positionXYZ = OffsetAroundY(x, y, z, -1.1f * s, 2.2f * s, 0.2f * s, rotY); // rotated around the Y axis by rotY so pieces of camera turn as one
	SetTransformations(scaleXYZ, 0, rotY, 0, positionXYZ);
	SetShaderColor(0.3f, 0.3f, 0.32f, 1.0f); // gray
	SetShaderMaterial("plastic"); 
	m_basicMeshes->DrawCylinderMesh();

	// Mode dial - cylinder
	scaleXYZ = glm::vec3(0.35f * s, 0.15f * s, 0.35f * s); // very small height
	positionXYZ = OffsetAroundY(x, y, z, 1.1f * s, 2.2f * s, 0.0f, rotY); // rotated around the Y axis by rotY so pieces of camera turn as one
	SetTransformations(scaleXYZ, 0, rotY, 0, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f); // black
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawCylinderMesh();

	// Lens mount - cylinder
	scaleXYZ = glm::vec3(0.95f * s, 0.35f * s, 0.95f * s);
	positionXYZ = OffsetAroundY(x, y, z, 0.0f, bodyMidY * s, 0.8f * s, rotY); // rotated around the Y axis by rotY so pieces of camera turn as one
	SetTransformations(scaleXYZ, 90.0f, 0, -rotY, positionXYZ); // lens lies on its side (X 90), so Z rotation turns it with the camera
	SetShaderMaterial("gold"); // metal reflection
	SetShaderColor(0.15f, 0.15f, 0.17f, 1.0f); // dark gray
	m_basicMeshes->DrawCylinderMesh();

	// Lens barrel - cylinder
	scaleXYZ = glm::vec3(0.8f * s, 0.7f * s, 0.8f * s);
	positionXYZ = OffsetAroundY(x, y, z, 0.0f, bodyMidY * s, 1.15f * s, rotY); // rotated around the Y axis by rotY so pieces of camera turn as one
	SetTransformations(scaleXYZ, 90.0f, 0, -rotY, positionXYZ); // lens lies on its side (X 90), so Z rotation turns it with the camera
	SetShaderMaterial("plastic");
	SetShaderTexture("bplastic"); // black plastic texture
	m_basicMeshes->DrawCylinderMesh();

	// Lens - cylinder
	scaleXYZ = glm::vec3(0.55f * s, 0.05f * s, 0.55f * s);
	positionXYZ = OffsetAroundY(x, y, z, 0.0f, bodyMidY * s, 1.85f * s, rotY); // rotated around the Y axis by rotY so pieces of camera turn as one
	SetTransformations(scaleXYZ, 90.0f, 0, -rotY, positionXYZ); // lens lies on its side (X 90), so Z rotation turns it with the camera
	SetShaderColor(0.95f, 0.95f, 0.95f, 0.3f); // clear color like lightstick bulb
	SetShaderMaterial("glass"); // reflective pane
	m_basicMeshes->DrawCylinderMesh();
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
	// load the textures for the 3D scene
	LoadSceneTextures();
	// define materials
	DefineObjectMaterials();
	// setup scene lights
	SetupSceneLights();
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadPrismMesh();
	m_basicMeshes->LoadConeMesh();
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

	// Bookshelf
	// Main plane
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(14.0f, 1.0f, 6.0f);
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 2.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ); // no rotation needed
	SetShaderColor(1, 1, 1, 1); // white
	SetShaderMaterial("wood"); // not very reflective
	SetShaderTexture("shelf"); // shelf texure
	SetTextureUVScale(2.0f, 1.0f); // tiling technique 2 across 1 down (for texturing)
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	SetTextureUVScale(1.0f, 1.0f); // set back to normal

	// Back wall
	scaleXYZ = glm::vec3(29.0f, 12.0f, 0.4f); // width, height, depth
	positionXYZ = glm::vec3(0.0f, 6.0f, -4.2f);  // moves position
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ); // no rotation needed
	// set the color white
	SetShaderColor(1, 1, 1, 1);
	// set material
	SetShaderMaterial("wood");
	// shelf texture
	SetShaderTexture("shelf");
	// draw mesh with new transformation
	m_basicMeshes->DrawBoxMesh();

	// Left wall
	scaleXYZ = glm::vec3(0.5f, 12.0f, 12.0f); // width, height, depth
	positionXYZ = glm::vec3(-14.2f, 6.0f, 2.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ); // no rotation needed
	// set the color white
	SetShaderColor(1, 1, 1, 1);
	// set material
	SetShaderMaterial("wood");
	// shelf texture
	SetShaderTexture("shelf");
	// draw mesh with new transformation
	m_basicMeshes->DrawBoxMesh();

	// Right shelf wall
	scaleXYZ = glm::vec3(0.5f, 12.0f, 12.0f); // width, height, depth
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(14.2f, 6.0f, 2.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ); // no rotation needed
	SetShaderColor(1, 1, 1, 1); // white	
	SetShaderMaterial("wood");  // set material
	SetShaderTexture("shelf"); // shelf texture
	m_basicMeshes->DrawBoxMesh(); // draw mesh with new transformation

	// Top of shelf
	scaleXYZ = glm::vec3(29.0f, 0.5f, 12.5f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 12.2f, 1.8f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ); // no rotation needed
	SetShaderColor(1, 1, 1, 1); 		// set the color white
	SetShaderMaterial("wood");			// set material
	SetShaderTexture("shelf");			// shelf texture
	m_basicMeshes->DrawBoxMesh();		// draw mesh with new transformation

	// Object function calls
	// x, y, z, scalar
	// or x y z yRot scalar
	DrawLightstick(-0.5f, 0.0f, 1.0f, 1.0f);
	DrawNotebook(-4.5f, 0.0f, 4.5f, 15.0f, 1.25f); // yRot
	DrawCandle(-10.0f, 0.0f, -1.5f, 1.0);
	DrawCat(7.0f, 0.0f, 0.5f, 2.0f);
	DrawCamera(12.0f, 0.0f, 6.0f, -50.0f, 1.0f); // yRot
}
