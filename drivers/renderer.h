// drivers/renderer.h
#ifndef RENDERER_H
#define RENDERER_H

/**
 *      Renderer Interface
 */
class Renderer{
    virtual void init() = 0;
    virtual void render() = 0;
    virtual void cleanup() = 0;
};

#endif