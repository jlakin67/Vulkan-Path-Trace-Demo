#include <iostream>
#include "VulkanApp.h"

int main()
{
    VulkanApp app;
    try {
        app.run();
    }
    catch (std::exception& e) {
        std::cout << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
