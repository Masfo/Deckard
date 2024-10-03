export module opengl;

// GL
namespace gl
{
	PFNGLCREATESHADERPROC            CreateShader;
	PFNGLSHADERSOURCEPROC            ShaderSource;
	PFNGLCREATEPROGRAMPROC           CreateProgram;
	PFNGLCOMPILESHADERPROC           CompileShader;
	PFNGLATTACHSHADERPROC            AttachShader;
	PFNGLLINKPROGRAMPROC             LinkProgram;
	PFNGLGETUNIFORMLOCATIONPROC      GetUniformLocation;
	PFNGLUSEPROGRAMPROC              UseProgram;
	PFNGLGENVERTEXARRAYSPROC         GenVertexArrays;
	PFNGLBINDVERTEXARRAYPROC         BindVertexArray;
	PFNGLGENBUFFERSPROC              GenBuffers;
	PFNGLBINDBUFFERPROC              BindBuffer;
	PFNGLBUFFERDATAPROC              BufferData;
	PFNGLVERTEXATTRIBPOINTERPROC     VertexAttribPointer;
	PFNGLENABLEVERTEXATTRIBARRAYPROC EnableVertexAttribArray;
	PFNGLUNIFORM1FPROC               Uniform1f;
	PFNGLGETSTRINGIPROC              GetStringi;

	// debug
	PFNGLDEBUGMESSAGECONTROLPROC  DebugMessageControl;
	PFNGLDEBUGMESSAGECALLBACKPROC DebugMessageCallback;
	PFNGLGETPROGRAMIVPROC         GetProgramiv;
	PFNGLGETPROGRAMINFOLOGPROC    GetProgramInfoLog;
	PFNGLGETSHADERIVPROC          GetShaderiv;
	PFNGLGETSHADERINFOLOGPROC     GetShaderInfoLog;

	PFNWGLSWAPINTERVALEXTPROC         wglSwapIntervalEXT;
	PFNWGLGETSWAPINTERVALEXTPROC      wglGetSwapIntervalEXT;
	PFNWGLGETEXTENSIONSSTRINGEXTPROC  wglGetExtensionsStringEXT;
	PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;


	std::unordered_set<std::string_view> extensions;
	std::unordered_set<std::string_view> wgl_extensions;

	bool has_gl_extension(std::string_view name) { return extensions.contains(name); }

	bool has_wgl_extension(std::string_view name) { return wgl_extensions.contains(name); }
} // namespace gl

GLuint compile_shader(GLenum type, const char* source)
{
	GLuint shader = gl::CreateShader(type);
	gl::ShaderSource(shader, 1, &source, NULL);
	gl::CompileShader(shader);

	GLint param;
	gl::GetShaderiv(shader, GL_COMPILE_STATUS, &param);
	if (!param)
	{
		char log[4096]{};
		gl::GetShaderInfoLog(shader, sizeof(log), NULL, log);
		dbg::println("Compile: {}", log);
	}

	return shader;
}

GLuint link_program(GLuint vert, GLuint frag)
{
	//
	GLuint program = gl::CreateProgram();
	gl::AttachShader(program, vert);
	gl::AttachShader(program, frag);
	gl::LinkProgram(program);

	GLint param;
	gl::GetProgramiv(program, GL_LINK_STATUS, &param);
	if (!param)
	{
		char log[4096]{};
		gl::GetProgramInfoLog(program, sizeof(log), NULL, log);
		dbg::println("Link: {}", log);
	}

	return program;
}

void init_gl_renderer()
{
	PIXELFORMATDESCRIPTOR pdf = {
	  .nSize        = sizeof(pdf),
	  .nVersion     = 1,
	  .dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
	  .iPixelType   = PFD_TYPE_RGBA,
	  .cColorBits   = 32,
	  .cDepthBits   = 24,
	  .cStencilBits = 8,
	  .iLayerType   = PFD_MAIN_PLANE,
	};
	if (SetPixelFormat(dc, ChoosePixelFormat(dc, &pdf), &pdf) == 0)
	{
		dbg::println("SetPixelFormat failed: {}", get_windows_error());
		return;
	}

	HGLRC old = wglCreateContext(dc);

	if (old == nullptr)
	{
		dbg::println("wglCreateContext failed: {}", get_windows_error());
		return;
	}

	if (wglMakeCurrent(dc, old) == 0)
	{
		dbg::println("wglMakeCurrent failed: {}", get_windows_error());
		return;
	}


	int attribs[] = {
	  // WGL_CONTEXT_MAJOR_VERSION_ARB,
	  // 4,
	  // WGL_CONTEXT_MINOR_VERSION_ARB,
	  // 6,
	  WGL_CONTEXT_PROFILE_MASK_ARB,
	  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
	  WGL_CONTEXT_FLAGS_ARB,
#ifdef _DEBUG
	  WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB | WGL_CONTEXT_DEBUG_BIT_ARB,
#else
	  WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,

#endif
	  0};

	// wgl
	gl::wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
	gl::wglGetExtensionsStringEXT  = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)wglGetProcAddress("wglGetExtensionsStringEXT");


	HGLRC hglrc = gl::wglCreateContextAttribsARB(dc, old, attribs);
	if (hglrc == nullptr)
	{
		dbg::println("wglCreateContextAttribs failed: {}", glGetError());
		return;
	}

	if (wglMakeCurrent(dc, hglrc) == 0)
	{
		dbg::println("wglMakeCurrent failed: {}", get_windows_error());
		return;
	}

	if (wglDeleteContext(old) == 0)
	{
		dbg::println("wglDeleteContext failed: {}", get_windows_error());
		return;
	}


	gl::CreateShader            = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
	gl::ShaderSource            = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
	gl::CompileShader           = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
	gl::CreateProgram           = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
	gl::AttachShader            = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
	gl::LinkProgram             = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
	gl::GetUniformLocation      = (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");
	gl::UseProgram              = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
	gl::GenVertexArrays         = (PFNGLGENVERTEXARRAYSPROC)wglGetProcAddress("glGenVertexArrays");
	gl::BindVertexArray         = (PFNGLBINDVERTEXARRAYPROC)wglGetProcAddress("glBindVertexArray");
	gl::GenBuffers              = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffers");
	gl::BindBuffer              = (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer");
	gl::BufferData              = (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData");
	gl::VertexAttribPointer     = (PFNGLVERTEXATTRIBPOINTERPROC)wglGetProcAddress("glVertexAttribPointer");
	gl::EnableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glEnableVertexAttribArray");
	gl::Uniform1f               = (PFNGLUNIFORM1FPROC)wglGetProcAddress("glUniform1f");
	gl::GetStringi              = (PFNGLGETSTRINGIPROC)wglGetProcAddress("glGetStringi");

	gl::DebugMessageControl  = (PFNGLDEBUGMESSAGECONTROLPROC)wglGetProcAddress("glDebugMessageControl");
	gl::DebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKPROC)wglGetProcAddress("glDebugMessageCallback");
	gl::GetShaderiv          = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
	gl::GetShaderInfoLog     = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
	gl::GetProgramiv         = (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv");
	gl::GetProgramInfoLog    = (PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog");

	if (gl::wglGetExtensionsStringEXT)
	{
		const std::string_view extensions = gl::wglGetExtensionsStringEXT();
		for (const auto& extension : split(extensions))
			gl::wgl_extensions.insert(extension);
	}


	dbg::println("GL Vendor   : {}", (const char*)glGetString(GL_VENDOR));
	dbg::println("GL Renderer : {}", (const char*)glGetString(GL_RENDERER));
	dbg::println("GL Version  : {}", (const char*)glGetString(GL_VERSION));
	dbg::println("GL GLSL     : {}", (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));


	int num_ext{};
	glGetIntegerv(GL_NUM_EXTENSIONS, &num_ext);
	for (int i = 0; i < num_ext; i++)
	{
		std::string_view ext = as<const char*>(gl::GetStringi(GL_EXTENSIONS, i));
		gl::extensions.insert(ext);
	}

	if (gl::has_wgl_extension("WGL_EXT_swap_control"))
	{
		gl::wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");

		gl::wglGetSwapIntervalEXT = (PFNWGLGETSWAPINTERVALEXTPROC)wglGetProcAddress("wglGetSwapIntervalEXT");

		if (gl::wglSwapIntervalEXT)
			gl::wglSwapIntervalEXT(1);
	}


	int flags{};
	glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
	if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
	{
		glEnable(GL_DEBUG_OUTPUT);

		gl::DebugMessageCallback(
		  [](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
		  {
			  (length);
			  (userParam);
			  (id);

			  std::string_view source_sv{"Unknown"};
			  switch (source)
			  {
				  case GL_DEBUG_SOURCE_API: source_sv = "API"; break;
				  case GL_DEBUG_SOURCE_WINDOW_SYSTEM: source_sv = "Window system"; break;
				  case GL_DEBUG_SOURCE_SHADER_COMPILER: source_sv = "Shader compiler"; break;
				  case GL_DEBUG_SOURCE_THIRD_PARTY: source_sv = "Third party"; break;
				  case GL_DEBUG_SOURCE_APPLICATION: source_sv = "Application"; break;
				  case GL_DEBUG_SOURCE_OTHER: source_sv = "Application"; break;
				  default: break;
			  }

			  std::string_view type_sv{"Unknown"};
			  switch (type)
			  {
				  case GL_DEBUG_TYPE_ERROR: type_sv = "Error"; break;
				  case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: type_sv = "Deprecated behavior"; break;
				  case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: type_sv = "Undefined behavior"; break;
				  case GL_DEBUG_TYPE_PORTABILITY: type_sv = "Portability"; break;
				  case GL_DEBUG_TYPE_PERFORMANCE: type_sv = "Performance"; break;
				  case GL_DEBUG_TYPE_OTHER: type_sv = "Other"; break;

				  default: break;
			  }

			  std::string_view severity_sv{"Unknown"};
			  switch (severity)
			  {
				  case GL_DEBUG_SEVERITY_NOTIFICATION: severity_sv = "Notify"; break;
				  case GL_DEBUG_SEVERITY_LOW: severity_sv = "Low"; break;
				  case GL_DEBUG_SEVERITY_MEDIUM: severity_sv = "Medium"; break;
				  case GL_DEBUG_SEVERITY_HIGH: severity_sv = "High"; break;

				  default: break;
			  }
			  dbg::println("{}: {} - {} | {}", source_sv, type_sv, severity_sv, message);
		  },
		  0);
		gl::DebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, 0, GL_TRUE);

#if 1
		gl::DebugMessageControl(

		  GL_DEBUG_SOURCE_APPLICATION, // source
		  GL_DONT_CARE,                // type
		  GL_DONT_CARE,                // severity
		  0,
		  0,
		  GL_FALSE);
#endif
	}


	const char* vert_shader =
	  "#version 330\n"
	  "layout(location = 0) in vec2 point;\n"
	  "uniform float angle;\n"
	  "void main() {\n"
	  "    mat2 rotate = mat2(cos(angle), -sin(angle),\n"
	  "                       sin(angle), cos(angle));\n"
	  "    gl_Position = vec4(0.75 * rotate * point, 0.0, 1.0);\n"
	  "}\n";
	const char* frag_shader =
	  "#version 330\n"
	  "out vec4 color;\n"
	  "void main() {\n"
	  "    color = vec4(0.75, 0.15, 0.15, 0);\n"
	  "}\n";
	GLuint vert    = compile_shader(GL_VERTEX_SHADER, vert_shader);
	GLuint frag    = compile_shader(GL_FRAGMENT_SHADER, frag_shader);
	GLuint program = link_program(vert, frag);
	gl::UseProgram(program);

	//
	u_angle = gl::GetUniformLocation(program, "angle");

	float  SQUARE[] = {-1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f};
	GLuint vao_point;
	gl::GenVertexArrays(1, &vao_point);
	gl::BindVertexArray(vao_point);

	GLuint vbo_point;
	gl::GenBuffers(1, &vbo_point);
	gl::BindBuffer(GL_ARRAY_BUFFER, vbo_point);
	gl::BufferData(GL_ARRAY_BUFFER, sizeof(SQUARE), SQUARE, GL_STATIC_DRAW);
	gl::VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	gl::EnableVertexAttribArray(0);
	gl::BindBuffer(GL_ARRAY_BUFFER, 0);

	glClearColor(0.0f, 0.5f, 0.75f, 1);

	renderer_initialized = true;
}

void close_renderer()
{
	if (auto hglrc = wglGetCurrentContext())
	{
		wglMakeCurrent(NULL, NULL);
		wglDeleteContext(hglrc);
	}
}

void render()
{
#if 1
	if (not vulkan.draw())
		return;
#else

#ifdef _DEBUG
	if (not renderer_initialized)
		return;
#endif
	glClear(GL_COLOR_BUFFER_BIT);
	gl::Uniform1f(u_angle, angle += 0.01);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	// glMultiDrawArrays, SSBO

	SwapBuffers(dc);

#endif
}

/*
	case VK_F4:
					{
						const int toggle_swap = 1 - gl::wglGetSwapIntervalEXT();
						gl::wglSwapIntervalEXT(toggle_swap);

						dbg::println("Toggle vsync: {}", toggle_swap);

						break;

*/
