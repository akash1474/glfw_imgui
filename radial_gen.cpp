// Declare a struct to hold the starting and ending points of the gradient
struct GradientPoints
{
    ImVec2 start;
    ImVec2 end;
};

// Declare a struct to hold the two colors of the gradient
struct GradientColors
{
    ImVec4 color1;
    ImVec4 color2;
};

// Function to generate the radial gradient texture
GLuint GenerateRadialGradientTexture(GradientPoints points, GradientColors colors, int width, int height)
{
    // Create a new OpenGL texture
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // Allocate memory for the texture data
    unsigned char* data = new unsigned char[width * height * 4];

    // Calculate the difference between the starting and ending points
    ImVec2 diff = points.end - points.start;

    // Calculate the length of the gradient line
    float length = sqrt(diff.x * diff.x + diff.y * diff.y);

    // Generate the texture data
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            // Calculate the distance from the start of the gradient line
            float distance = ImDot(ImVec2((float)x, (float)y) - points.start, diff) / length;

            // Clamp the distance to the range [0, 1]
            distance = ImClamp(distance, 0.0f, 1.0f);

            // Interpolate the colors using the distance
            ImVec4 color = ImLerp(colors.color1, colors.color2, distance);

            // Set the texture data for this pixel
            data[(y * width + x) * 4 + 0] = (unsigned char)(color.x * 255.0f);
            data[(y * width + x) * 4 + 1] = (unsigned char)(color.y * 255.0f);
            data[(y * width + x) * 4 + 2] = (unsigned char)(color.z * 255.0f);
            data[(y * width + x) * 4 + 3] = (unsigned char)(color.w * 255.0f);
        }
    }

    // Set the texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Upload the texture data to the GPU
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    // Free the texture data
    delete[] data;

    return texture;
}

int main()
{
    // Initialize ImGui and OpenGL
    // ...

    // Set the starting and ending points of the gradient
    GradientPoints points;
    points.start = ImVec2(50.0f, 50.0f);
    points.end = ImVec2(200.0f, 200.0f);

    // Set the gradient colors
    GradientColors colors;
    colors.color1 = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red
    colors.color2 = ImVec4(0.0f, 0.0f, 1.0f, 1.0f); // Blue

    // Generate the gradient texture
    GLuint texture = GenerateRadialGradientTexture(points, colors, 256, 256);

    // Main loop
    while (true)
    {
        // Update ImGui and OpenGL
        // ...

        // Begin a new ImGui window
        ImGui::Begin("Radial Gradient");

        // Render the gradient in the window
        RenderGradient(texture, 256, 256);

        // End the ImGui window
        ImGui::End();

        // Render ImGui and OpenGL
        // ...
    }

    // Shut down ImGui and OpenGL
    // ...

    return 0;
}