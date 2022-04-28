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
	<li>Install CMake, Python</li>
	<li>git clone --recursive https://github.com/youss2017/ExoabEngine</li>
	<li>Go to ExoabEngine/ExoabEngine/external/</li>
	<li>Move all dependices (dll, lib) to bin/dep for the following items.</li>
	<li>Compiling Assimp</li>
	<li>Compiling GLFW</li>
</ol>
</p>
</body>

</html>
