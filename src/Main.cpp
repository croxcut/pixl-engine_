#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <stdio.h>

int main(void) {
    
    if(!glfwInit()) 
        printf("Not initialized");
    else
        printf("GLFW initialized!\n");
    
    return 0;
}