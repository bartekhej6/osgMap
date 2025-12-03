#include <osgGA/CameraManipulator>
#include <osgGA/GUIEventAdapter>
#include <osgGA/GUIActionAdapter>
#include <osg/Matrix>
#include <osg/Vec3d>
#include <osg/BoundingSphere>
#include <osg/Node>
#include <algorithm>

class GoogleMapsManipulator : public osgGA::CameraManipulator {
public:
    GoogleMapsManipulator()
        : _distance(100.0), _lastX(0), _lastY(0), _center(0, 0, 1)
    {} // Non-zero center avoids a degenerate up/eye direction

    void resetFromBounds()
    {
        if (_node.valid())
        {
            _center = _node->getBound().center();
            _distance = std::max(100.0, _node->getBound().radius() * 2.5);
        }
    }

    void setNode(osg::Node* node) override
    {
        _node = node;
        osgGA::CameraManipulator::setNode(node);
        resetFromBounds();
    }
    void home(double) override { resetFromBounds(); }
    void home(const osgGA::GUIEventAdapter&, osgGA::GUIActionAdapter&) override
    {
        resetFromBounds();
    }

    // Build the view matrix each frame: eye along the center direction at a given distance, global Z as up.
    osg::Matrixd getInverseMatrix() const override
    {
        osg::Vec3d eye = _center;
        eye.normalize();
        eye *= (_center.length() + _distance);
        return osg::Matrixd::lookAt(eye, _center, osg::Vec3d(0, 0, 1));
    }

    // Forward matrix required by the base class.
    osg::Matrixd getMatrix() const override
    {
        return osg::Matrixd::inverse(getInverseMatrix());
    }

    // Unused matrix setters for this manipulator.
    void setByMatrix(const osg::Matrixd&) override {}
    void setByInverseMatrix(const osg::Matrixd&) override {}

    bool handle(const osgGA::GUIEventAdapter& ea,
                osgGA::GUIActionAdapter& aa) override
    {
        if (ea.getEventType() == osgGA::GUIEventAdapter::PUSH)
        {
            // Remember cursor position to compute drag deltas.
            _lastX = ea.getXnormalized();
            _lastY = ea.getYnormalized();
            return true;
        }
        if (ea.getEventType() == osgGA::GUIEventAdapter::DRAG
            && (ea.getButtonMask() & osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON))
        {
            // Local tangent frame: up (center direction), east (global Z × up), north (up × east).
            osg::Vec3d up = _center;
            up.normalize();
            osg::Vec3d east = osg::Vec3d(0, 0, 1) ^ up;
            east.normalize();
            osg::Vec3d north = up ^ east;

            float x = ea.getXnormalized(), y = ea.getYnormalized();
            _center -= (east * (x - _lastX) * _distance)
                + (north * (y - _lastY) * _distance);
            _lastX = x;
            _lastY = y;
            aa.requestRedraw();
            return true;
        }
        if (ea.getEventType() == osgGA::GUIEventAdapter::SCROLL)
        {
            // Zoom toward/away from the center.
            _distance *=
                (ea.getScrollingMotion() == osgGA::GUIEventAdapter::SCROLL_UP
                     ? 0.8
                     : 1.25);
            _distance = std::max(_distance, 10.0);
            aa.requestRedraw();
            return true;
        }
        // Home key resets center/distance to node bounds.
        return (ea.getEventType() == osgGA::GUIEventAdapter::KEYDOWN
                && ea.getKey() == osgGA::GUIEventAdapter::KEY_Home)
            ? (resetFromBounds(), aa.requestRedraw(), true)
            : false;
    }

private:
    osg::observer_ptr<osg::Node> _node;
    osg::Vec3d _center;
    double _distance;
    float _lastX, _lastY;
};