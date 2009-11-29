/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
 */
#include "Waypoints.hpp"
#include "WaypointVisitor.hpp"
#include "Navigation/TaskProjection.hpp"
#include <vector>

#ifdef INSTRUMENT_TASK
unsigned n_queries = 0;
extern long count_intersections;
#endif

Waypoints::Waypoints()
{
}

void
Waypoints::optimise()
{
  task_projection.update_fast();

  while (!tmp_wps.empty()) {
    Waypoint w = (tmp_wps.front());
    w.FlatLocation = task_projection.project(w.Location);
    waypoint_tree.insert(w);
    tmp_wps.pop_front();
  }

  waypoint_tree.optimize();
}

void
Waypoints::insert(const Waypoint& wp)
{
  if (tmp_wps.empty() && waypoint_tree.empty()) {
    task_projection.reset(wp.Location);
  }
  task_projection.scan_location(wp.Location);

  tmp_wps.push_back(wp);

  /**
   * \todo
   * if range changed, need to re-pack waypoints
   * will have to remove all from the list, recalculate projections,
   * then add them again!
   * (can just insert() them all, then clear the tree, then run optimise()
   */
}



Waypoints::WaypointTree::const_iterator 
Waypoints::find_nearest(const GEOPOINT &loc) const 
{
  FLAT_GEOPOINT floc = task_projection.project(loc);
  Waypoint bb_target; bb_target.FlatLocation = floc;
  std::pair<WaypointTree::const_iterator, double> 
    found = waypoint_tree.find_nearest(bb_target);

#ifdef INSTRUMENT_TASK
  n_queries++;
#endif

  return found.first;
}

const Waypoint*
Waypoints::lookup_id(const unsigned id) const
{
  WaypointTree::const_iterator found = waypoint_tree.begin();
  while (found != waypoint_tree.end()) {
    if ((*found).id == id) {
      return &(*found);
    }
    found++;
  }
  return NULL;
}


Waypoints::WaypointTree::const_iterator
Waypoints::find_id(const unsigned id) const
{
  WaypointTree::const_iterator found = waypoint_tree.begin();
  while (found != waypoint_tree.end()) {
    if ((*found).id == id) {
      break;
    }
    found++;
  }
#ifdef INSTRUMENT_TASK
  n_queries++;
#endif

  return found;
}


std::vector< Waypoint >
Waypoints::find_within_range(const GEOPOINT &loc, 
                             const double range) const
{
  FLAT_GEOPOINT floc = task_projection.project(loc);
  Waypoint bb_target; bb_target.FlatLocation = floc;
  const unsigned mrange = task_projection.project_range(loc, range);

  std::vector< Waypoint > vectors;
  waypoint_tree.find_within_range(bb_target, mrange, 
                                  std::back_inserter(vectors));
#ifdef INSTRUMENT_TASK
  n_queries++;
#endif
  return vectors;
}


void 
Waypoints::visit_within_range(const GEOPOINT &loc, 
                              const double range,
                              WaypointVisitor& visitor) const
{
  FLAT_GEOPOINT floc = task_projection.project(loc);
  Waypoint bb_target; bb_target.FlatLocation = floc;
  const unsigned mrange = task_projection.project_range(loc, range);
  
  waypoint_tree.visit_within_range(bb_target, mrange, visitor);

#ifdef INSTRUMENT_TASK
  n_queries++;
#endif
}


std::vector< Waypoint >
Waypoints::find_within_range_circle(const GEOPOINT &loc, 
                                    const double range) const
{
  const unsigned mrange = task_projection.project_range(loc, range);
  std::vector < Waypoint > vectors = find_within_range(loc, range);
  FLAT_GEOPOINT floc = task_projection.project(loc);

  for (std::vector< Waypoint >::iterator v=vectors.begin();
       v != vectors.end(); ) {

#ifdef INSTRUMENT_TASK
        count_intersections++;
#endif

    if ((*v).FlatLocation.distance_to(floc)> mrange) {
      vectors.erase(v);
    } else {
      v++;
    }
  }
  return vectors;
}


Waypoints::WaypointTree::const_iterator
Waypoints::begin() const
{
  return waypoint_tree.begin();
}

Waypoints::WaypointTree::const_iterator
Waypoints::end() const
{
  return waypoint_tree.end();
}



void
Waypoints::clear()
{
  waypoint_tree.clear();
}

unsigned
Waypoints::size() const
{
  return waypoint_tree.size();
}
