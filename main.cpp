#include "trianglerenderer.h"
#include <stdio.h>

int main()
{

    TriangleRenderer triangleRender(0, 0);
    triangleRender.makeCurrent();
    triangleRender.initializeProgram();

    TriangleRenderer second(2,0);
    second.makeCurrent();
    second.initializeProgram();


    while (true) {
        triangleRender.makeCurrent();
        triangleRender.bindProgram();
        triangleRender.rotate(-1);
        triangleRender.renderFrame();
        triangleRender.swapBuffers();

        second.makeCurrent();
        second.bindProgram();
        second.rotate(1);
        second.renderFrame();
        second.swapBuffers();
        triangleRender.releaseProgram();
    }

    return 0;
}

