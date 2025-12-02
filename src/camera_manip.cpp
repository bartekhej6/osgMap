#include <osgGA/CameraManipulator>
#include <osgGA/GUIEventAdapter>
#include <osgGA/GUIActionAdapter>
#include <osg/Matrix>
#include <osg/Vec3d>
#include <osg/BoundingSphere>
#include <osg/Node>
#include <algorithm>
#include <cmath>
#include <iostream>

class GoogleMapsManipulator : public osgGA::CameraManipulator {
public:
    GoogleMapsManipulator()
        : _center(0.0, 0.0, 0.0), _distance(100.0), _lastX(0.0f), _lastY(0.0f)
    {
        std::cout << "Debug: GoogleMapsManipulator Version 2.0 (Math Logic)"
                  << std::endl;
    }

    const char* className() const override { return "GoogleMapsManipulator"; }

private:
    static constexpr double kTinyEpsilon2 = 1e-12;

    osg::Vec3d normalizeOr(const osg::Vec3d& v,
                           const osg::Vec3d& fallback) const
    {
        if (v.length2() <= kTinyEpsilon2) return fallback;
        osg::Vec3d out = v;
        out.normalize();
        return out;
    }

    // ECEF Up vector calculation
    osg::Vec3d getSurfaceNormal(const osg::Vec3d& point) const
    {
        if (point.length2() > 1000.0)
            return normalizeOr(point, osg::Vec3d(0.0, 0.0, 1.0));
        return osg::Vec3d(0.0, 0.0, 1.0);
    }

    // Tangent basis (North/East)
    void getTangentBasis(const osg::Vec3d& point, osg::Vec3d& outRight,
                         osg::Vec3d& outNorth) const
    {
        const osg::Vec3d up = getSurfaceNormal(point);
        const osg::Vec3d globalNorth(0.0, 0.0, 1.0);

        osg::Vec3d east = globalNorth ^ up;
        if (east.length2() <= kTinyEpsilon2)
            east = osg::Vec3d(1.0, 0.0, 0.0) ^ up;
        east.normalize();

        osg::Vec3d north = up ^ east;
        north = normalizeOr(north, globalNorth);

        outRight = east;
        outNorth = north;
    }

public:
    void setNode(osg::Node* node) override
    {
        _node = node;
        if (_node.valid()) home(0.0);
    }
    osg::Node* getNode() override { return _node.get(); }
    const osg::Node* getNode() const override { return _node.get(); }

    // Set camera by matrix (Eye -> Center logic)
    void setByMatrix(const osg::Matrixd& matrix) override
    {
        const osg::Vec3d eye = matrix.getTrans();
        osg::Vec3d lookDir =
            -osg::Vec3d(matrix(2, 0), matrix(2, 1), matrix(2, 2));
        lookDir.normalize();

        const osg::Vec3d planeNormal = getSurfaceNormal(_center);
        // Simple fallback projection for now
        _center = eye - (planeNormal * 1000.0);
        _distance = 1000.0;
    }

    void setByInverseMatrix(const osg::Matrixd& inv) override
    {
        setByMatrix(osg::Matrixd::inverse(inv));
    }
    osg::Matrixd getMatrix() const override
    {
        return osg::Matrixd::inverse(getInverseMatrix());
    }

    // Crucial: Constructs the LookAt matrix using our ECEF logic
    osg::Matrixd getInverseMatrix() const override
    {
        const osg::Vec3d up = getSurfaceNormal(_center);
        const osg::Vec3d eye = _center + (up * _distance);

        osg::Vec3d right, north;
        getTangentBasis(_center, right, north);
        return osg::Matrixd::lookAt(eye, _center, north);
    }

    void home(double) override
    {
        if (!_node) return;
        _center = _node->getBound().center();
        _distance = std::max(100.0, _node->getBound().radius() * 2.5);
    }
    void home(const osgGA::GUIEventAdapter&, osgGA::GUIActionAdapter&) override
    {
        home(0.0);
    }
    bool handle(const osgGA::GUIEventAdapter&,
                osgGA::GUIActionAdapter&) override
    {
        return false;
    }

private:
    osg::observer_ptr<osg::Node> _node;
    osg::Vec3d _center;
    double _distance;
    float _lastX, _lastY;
};