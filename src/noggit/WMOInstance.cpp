// WMOInstance.cpp is part of Noggit3, licensed via GNU General Public License (version 3).
// Bernd Lörwald <bloerwald+noggit@googlemail.com>
// Stephan Biegel <project.modcraft@googlemail.com>
// Tigurius <bstigurius@googlemail.com>

#include <noggit/WMOInstance.h>

#include <noggit/Log.h>
#include <noggit/MapHeaders.h>
#include <noggit/WMO.h> // WMO
#include <noggit/World.h>
#include <noggit/mpq/file.h>

WMOInstance::WMOInstance(World* world, std::string const& path, noggit::mpq::file* _file )
  : wmo (world, path)
  , mSelectionID( world->selection_names().add( this ) )
  , _world (world)
{
  _file->read( &mUniqueID, 4 );
  _file->read( &pos, 12 );
  _file->read( &dir, 12 );
  _file->read( &extents[0], 12 );
  _file->read( &extents[1], 12 );
  _file->read( &mFlags, 2 );
  _file->read( &doodadset, 2 );
  _file->read( &mNameset, 2 );
  _file->read( &mUnknown, 2 );
}

WMOInstance::WMOInstance( World* world, std::string const& path, ENTRY_MODF* d )
  : wmo (world, path)
  , pos( d->pos[0], d->pos[1], d->pos[2] )
  , dir( d->rot[0], d->rot[1], d->rot[2] )
  , mUniqueID( d->uniqueID )
  , mFlags( d->flags )
  , mUnknown( d->unknown )
  , mNameset( d->nameSet )
  , doodadset( d->doodadSet )
  , mSelectionID( world->selection_names().add( this ) )
  , _world (world)
{
  extents[0] = ::math::vector_3d( d->extents[0][0], d->extents[0][1], d->extents[0][2] );
  extents[1] = ::math::vector_3d( d->extents[1][0], d->extents[1][1], d->extents[1][2] );
}

WMOInstance::WMOInstance( World* world, std::string const& path )
  : wmo (world, path)
  , pos( 0.0f, 0.0f, 0.0f )
  , dir( 0.0f, 0.0f, 0.0f )
  , mUniqueID( 0 )
  , mFlags( 0 )
  , mUnknown( 0 )
  , mNameset( 0 )
  , doodadset( 0 )
  , mSelectionID( world->selection_names().add( this ) )
  , _world (world)
{
}

void WMOInstance::draw ( bool draw_doodads
                       , bool draw_fog
                       , bool hasSkies
                       , const float culldistance
                       , const float& fog_distance
                       , const Frustum& frustum
                       , const ::math::vector_3d& camera
                       , const boost::optional<selection_type>& selected_item
                       ) const
{
  glPushMatrix();
  glTranslatef( pos.x(), pos.y(), pos.z() );

  const float roty = dir.y() - 90.0f;

  glRotatef( roty, 0.0f, 1.0f, 0.0f );
  glRotatef( -dir.x(), 0.0f, 0.0f, 1.0f );
  glRotatef( dir.z(), 1.0f, 0.0f, 0.0f );

  const bool is_selected ( selected_item
                        && noggit::selection::is_the_same_as ( this
                                                             , *selected_item
                                                             )
                         );

  wmo->draw ( _world
            , doodadset
            , pos
            , roty
            , culldistance
            , is_selected
            , is_selected
            , is_selected
            , draw_doodads
            , draw_fog
            , hasSkies
            , fog_distance
            , frustum
            , camera
            );

  glPopMatrix();
}

void WMOInstance::drawSelect ( bool draw_doodads
                             , const float culldistance
                             , const Frustum& frustum
                             , const ::math::vector_3d& camera
                             ) const
{
  glPushMatrix();

  glTranslatef( pos.x(), pos.y(), pos.z() );

  const float roty = dir.y() - 90.0f;

  glRotatef( roty, 0.0f, 1.0f, 0.0f );
  glRotatef( -dir.x(), 0.0f, 0.0f, 1.0f );
  glRotatef( dir.z(), 1.0f, 0.0f, 0.0f );

  //mSelectionID = _world->selection_names().add( this );
  glPushName( mSelectionID );

  wmo->drawSelect ( _world
                  , doodadset
                  , pos
                  , -roty
                  , culldistance
                  , draw_doodads
                  , frustum
                  , camera
                  );

  glPopName();

  glPopMatrix();
}

/*void WMOInstance::drawPortals()
{
  glPushMatrix();

  glTranslatef( pos.x(), pos.y(), pos.z() );

  const float roty = dir.y() - 90.0f;

  glRotatef( roty, 0.0f, 1.0f, 0.0f );
  glRotatef( -dir.x(), 0.0f, 0.0f, 1.0f );
  glRotatef( dir.z(), 1.0f, 0.0f, 0.0f );

  wmo->drawPortals();

  glPopMatrix();
}*/

void WMOInstance::resetDirection()
{
  dir = ::math::vector_3d( 0.0f, dir.y(), 0.0f );
}

WMOInstance::~WMOInstance()
{
  _world->selection_names().del( mSelectionID );
}

namespace
{
  void maybe_expand (::math::vector_3d* extents, ::math::vector_3d new_point)
  {
    extents[0] = ::math::min (extents[0], new_point);
    extents[1] = ::math::max (extents[1], new_point);
  }
}

void WMOInstance::recalc_extents()
{
  ::math::matrix_4x4 const rot
    ( ::math::matrix_4x4::new_translation_matrix (pos)
    * ::math::matrix_4x4::new_rotation_matrix (dir)
    );

  extents[0] = ::math::vector_3d::max();
  extents[1] = ::math::vector_3d::min();

  maybe_expand (extents, rot * ::math::vector_3d (wmo->extents[0].x(), wmo->extents[0].y(), wmo->extents[0].z()));
  maybe_expand (extents, rot * ::math::vector_3d (wmo->extents[0].x(), wmo->extents[0].y(), wmo->extents[1].z()));
  maybe_expand (extents, rot * ::math::vector_3d (wmo->extents[0].x(), wmo->extents[1].y(), wmo->extents[0].z()));
  maybe_expand (extents, rot * ::math::vector_3d (wmo->extents[0].x(), wmo->extents[1].y(), wmo->extents[1].z()));
  maybe_expand (extents, rot * ::math::vector_3d (wmo->extents[1].x(), wmo->extents[0].y(), wmo->extents[0].z()));
  maybe_expand (extents, rot * ::math::vector_3d (wmo->extents[1].x(), wmo->extents[0].y(), wmo->extents[1].z()));
  maybe_expand (extents, rot * ::math::vector_3d (wmo->extents[1].x(), wmo->extents[1].y(), wmo->extents[0].z()));
  maybe_expand (extents, rot * ::math::vector_3d (wmo->extents[1].x(), wmo->extents[1].y(), wmo->extents[1].z()));

  for (std::size_t i (0); i < wmo->nGroups; ++i)
  {
    maybe_expand (extents, rot * ::math::vector_3d (wmo->groups[i].BoundingBoxMin.x(), wmo->groups[i].BoundingBoxMin.y(), wmo->groups[i].BoundingBoxMin.z()));
    maybe_expand (extents, rot * ::math::vector_3d (wmo->groups[i].BoundingBoxMin.x(), wmo->groups[i].BoundingBoxMin.y(), wmo->groups[i].BoundingBoxMax.z()));
    maybe_expand (extents, rot * ::math::vector_3d (wmo->groups[i].BoundingBoxMin.x(), wmo->groups[i].BoundingBoxMax.y(), wmo->groups[i].BoundingBoxMin.z()));
    maybe_expand (extents, rot * ::math::vector_3d (wmo->groups[i].BoundingBoxMin.x(), wmo->groups[i].BoundingBoxMax.y(), wmo->groups[i].BoundingBoxMax.z()));
    maybe_expand (extents, rot * ::math::vector_3d (wmo->groups[i].BoundingBoxMax.x(), wmo->groups[i].BoundingBoxMin.y(), wmo->groups[i].BoundingBoxMin.z()));
    maybe_expand (extents, rot * ::math::vector_3d (wmo->groups[i].BoundingBoxMax.x(), wmo->groups[i].BoundingBoxMin.y(), wmo->groups[i].BoundingBoxMax.z()));
    maybe_expand (extents, rot * ::math::vector_3d (wmo->groups[i].BoundingBoxMax.x(), wmo->groups[i].BoundingBoxMax.y(), wmo->groups[i].BoundingBoxMin.z()));
    maybe_expand (extents, rot * ::math::vector_3d (wmo->groups[i].BoundingBoxMax.x(), wmo->groups[i].BoundingBoxMax.y(), wmo->groups[i].BoundingBoxMax.z()));
  }
}