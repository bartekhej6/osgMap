#include <osgDB/ReadFile>
#include <osgUtil/Optimizer>
#include <osg/CoordinateSystemNode>

#include <osg/Switch>
#include <osg/Types>
#include <osgText/Text>
#include <osg/MatrixTransform>
#include <osg/ShapeDrawable>

#include <osgSim/ShapeAttribute>

#include <iostream>

#include "common.h"

using namespace osg;

// klasa typu NodeVisitor to wzorzec projektowy - warto znaæ!
// Zadaniem glasy jest odwiedziæ wszystkie wêz³y drzewa. Specjalizacja
// tej klasy mo¿e dokonaæ odpowiednich zmian np zmieniæ pozycjê wierzcho³ków,
// dodaæ now¹ geometriê, zmieniæ stany wyœwietlania itd
class GeomVisitor : public osg::NodeVisitor
{

public:

    GeomVisitor(/* add your params */) :
        osg::NodeVisitor( NodeVisitor::TRAVERSE_ALL_CHILDREN )
    {
    }
    //////////////////////////////////////////////////////////////////////////////
    void apply( osg::Node & node)
    {
        traverse(node);
    }
    //////////////////////////////////////////////////////////////////////////////
    void apply( osg::Geode & node)
    {
        for ( unsigned i=0; i<node.getNumDrawables(); i++ ) {
            osg::Geometry* geom = dynamic_cast <osg::Geometry*> (node.getDrawable(i));

            if ( !geom ) continue;

            osg::Vec3Array* verts = (osg::Vec3Array*) geom->getVertexArray();

            for ( unsigned j=0; j<verts->size(); j++ ) {
                // tutaj mo¿na zmodyfikowaæ wierzcho³ek
#if 0
                (*verts)[j] = osg::Vec3 ();
#endif
            }
        }
    }
};



void parse_meta_data(osg::Node* model)
{
    osg::Group * group = model->asGroup();
    GeomVisitor gv;

    for (unsigned i = 0; i < group->getNumChildren(); i++)
    {
        osg::Node* kido = group->getChild(i);
        osgSim::ShapeAttributeList * sal = (osgSim::ShapeAttributeList*)kido->getUserData();
        if (!sal)
            continue;

        float height = 0.f;
        for (unsigned j = 0; j < sal->size(); j++)
        {
            // wysokoœci budynkow - przechodzimy wszystkie atrybuty 
            // w poszukiwaniu atrybutu o nazwie "height"
            if ((*sal)[j].getName().find("height") != std::string::npos)
            {
                if ((*sal)[j].getType() == osgSim::ShapeAttribute::DOUBLE)
                    height = float((*sal)[j].getDouble()) / 100.f; // cm
                else if ((*sal)[j].getType() == osgSim::ShapeAttribute::INTEGER)
                    height = float((*sal)[j].getInt()) / 100.f; // cm
                break;
            }
        }

        kido->accept(gv);
    }
}


osg::Node* process_buildings(osg::Matrixd& ltw, const std::string & file_path)
{
    std::string buildings_file_path = file_path + "/buildings_levels.shp";

    // load the data
    osg::ref_ptr<osg::Node> buildings_model = osgDB::readRefNodeFile(buildings_file_path);
    if (!buildings_model)
    {
        std::cout << "Cannot load file " << buildings_file_path << std::endl;
        return nullptr;
    }

    // Transformacja ze wspó³rzêdnych geograficznych (GEO) do wspó³rzêdnych œwiata (WGS)
    ConvertFromGeoProjVisitor<true> cfgp;
    buildings_model->accept(cfgp);

    WorldToLocalVisitor ltwv(ltw, true);
    buildings_model->accept(ltwv);

#if 0
    // dokonuj dodatkowego przetwarzania wierzcho³ków po transformacji z uk³adu Geo do WGS
    parse_meta_data(buildings_model);
#endif






    // GOOD LUCK!









    return buildings_model.release();
}

