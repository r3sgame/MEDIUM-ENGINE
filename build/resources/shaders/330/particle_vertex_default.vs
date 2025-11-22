#version 330
// Input attributes from the VBOs
in vec3 vertexPosition;
// Now interpreted as: (size, r, g, b)
in vec4 vertexData; 

// Output to Fragment Shader
out vec4 fragColor; // Passing color data (r, g, b, life)

// Uniforms
uniform mat4 mvp; // raylib's model-view-projection matrix

void main()
{
    // Apply the combined matrix to get the final screen position
    gl_Position = mvp * vec4(vertexPosition, 1.0);
    
    // --- KEY MODIFICATION ---
    // 1. Get the custom size from the X component
    float size = vertexData.x; 
    
    // 2. Set the point size using the custom size parameter
    gl_PointSize = size; 

    // 3. Prepare the output color data (RGB from VBO, and the particle's life in A)
    //    We assume particle life is passed via a separate VBO or Uniform if needed.
    //    If LIFE is not passed, you need to add it as the 5th attribute (extra VBO) or re-purpose an existing component.
    
    // Assuming you stick to the 4 float input, we'll pass (R, G, B, 1.0) and assume life is handled on the CPU
    // OR we can assume you changed your CPU data packing to (size, r, g, life)
    // Let's assume you've used a separate attribute (like 'vertexLife') for life if you need it.
    
    // For this example, we'll assume the input attribute only contains (size, r, g, b)
    // and we'll use a hardcoded alpha/life in the Fragment Shader.
    fragColor = vec4(vertexData.y, vertexData.z, vertexData.w, 1.0);
}