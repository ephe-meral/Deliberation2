#pragma once

#include <Deliberation/Deliberation_API.h>

#include <Deliberation/Draw/SurfaceBinary.h>

namespace deliberation
{

class Context;
class SurfaceDownloadImpl;

class DELIBERATION_API SurfaceDownload final
{
public:
    SurfaceDownload();

    bool isDone() const;

    void start();

    const SurfaceBinary & result() const;

private:
    friend class Context;
    friend class Surface;

private:
    SurfaceDownload(const std::shared_ptr<SurfaceDownloadImpl> & impl);

private:
    std::shared_ptr<SurfaceDownloadImpl> m_impl;
};

}
