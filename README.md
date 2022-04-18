<html>

<body>
<h1>Note on debugging with RenderDOC</h1>
<p>Because VK_KHR_buffer_device_address (GPU Pointers) are used renderdoc does not support debugging this feature by default, therefore use debug commandline parameters,
to prevent the application form enabling that feature, this will allow RenderDOC to work with the cost of validation errors.</p>
<h1>TODO</h1>
<ul>
	<li>Convert to Physically Based Rendering</li>
	<li>Animation</li>
	<li>Fix Bloom Pass</li>
	<li>Cascaded Shadow Implementation</li>
	<li>Fix Terrain Shadow</li>
	<li>Terrain Culling</li>
	<li>Physics Implementation</li>
	<li><b>Maybe</b> Switch from :) VK_KHR_dynamic_renderpass to normal VkRenderPass :(</li>
</ul>

<p><h1>***IMPORTANT***</h1><b>Update Vulkan SDK C/C++ include directroy and Additional Library Directory.</b></p>
<h1>Build Instructions</h1>
<p>
<ol>
	<li>git clone --recursive https://github.com/youss2017/ExoabEngine</li>
	<li>Go to ExoabEngine/ExoabEngine/external/</li>
	<li>Compiling Assimp</li>
	<ul>
			<li>Open CMake-GUI, To compile assimp set output dir to cmake_files/assimp and check BUILD_SHARED_LIBS</li>
			<li>Open Project and set the following configurations</li>
			<li><b>Select Release Mode</b></li>
			<li>assimp-->properties-->General set "OutputDirectory" to $(ProjectDir)..\..\..\bin\</li>
			<li>assimp-->properties-->linker-->All Options set "Import Library" to $(ProjectDir)..\..\..\bin\debug\assimp-vc143-mt.lib</li>
			<li>assimp-->properties-->linker-->All Options set "Generate Program Database File" to $(ProjectDir)..\..\..\bin\assimp-vc143-mt.pdb</li>
		<li>Compile "ALL_BUILD" in release mode.</li>
	</ul>
	<li>Compiling GLFW</li>
	<ul>
		<li>Open CMake-GUI, To compile assimp set output dir to cmake_files/glfw and <b>***UNCHECK***</b> GLFW_BUILD_DOCS, GLFW_BUILD_EXAMPLES, GLFW_BUILD_TESTS. <b>***CHECK***</b>BUILD_SHARED_LIBS</li>
		<li>Open Project set the following configurations</li>
		<li><b>Select Release Mode</b></li>
		<li>glfw-->properties-->General set "OutputDirectory" to $(ProjectDir)..\..\..\bin\</li>
		<li>glfw-->properties-->linker-->All Options set "Import Library" to $(ProjectDir)..\..\..\bin\release\glfw3dll.lib</li>
		<li>glfw-->properties-->linker-->All Options set "Generate Program Database File" to $(ProjectDir)..\..\..\bin\release\glfw3.pdb</li>
		<li>Compile "ALL_BUILD" in release mode.</li>
	</ul>
</ol>
</p>
</body>

</html>
