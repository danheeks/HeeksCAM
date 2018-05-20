////////////////////////////////////////////////////////////////////////////////
// Filename: colorshaderclass.h
////////////////////////////////////////////////////////////////////////////////
#ifndef _COLORSHADERCLASS_H_
#define _COLORSHADERCLASS_H_


//////////////
// INCLUDES //
//////////////
#include <fstream>
using namespace std;


///////////////////////
// MY CLASS INCLUDES //
///////////////////////
#include "openglclass.h"


////////////////////////////////////////////////////////////////////////////////
// Class name: ColorShaderClass
////////////////////////////////////////////////////////////////////////////////
class ColorShaderClass
{
public:
	ColorShaderClass();
	ColorShaderClass(const ColorShaderClass&);
	~ColorShaderClass();

	bool Initialize(OpenGLClass*);
	void Shutdown(OpenGLClass*);
	void SetShader(OpenGLClass*);
	bool SetShaderParameters(OpenGLClass*, float*, float*, float*);

private:
	bool InitializeShader(char*, char*, OpenGLClass*);
	char* LoadShaderSourceFile(char*);
	void OutputShaderErrorMessage(OpenGLClass*, unsigned int, char*);
	void OutputLinkerErrorMessage(OpenGLClass*, unsigned int);
	void ShutdownShader(OpenGLClass*);

private:
	unsigned int m_vertexShader;
	unsigned int m_fragmentShader;
	unsigned int m_shaderProgram;
};

#endif