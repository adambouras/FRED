/*
  This file is part of the FRED system.

  Copyright (c) 2013-2016, University of Pittsburgh, John Grefenstette,
  David Galloway, Mary Krauland, Michael Lann, and Donald Burke.

  Based in part on FRED Version 2.9, created in 2010-2013 by
  John Grefenstette, Shawn Brown, Roni Rosenfield, Alona Fyshe, David
  Galloway, Nathan Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

//
//
// File: Neighborhood_Patch.cc
//


#include <new>
#include "Global.h"
#include "Geo.h"
#include "Neighborhood_Layer.h"
#include "Neighborhood_Patch.h"
#include "Person.h"
#include "Place.h"
#include "Neighborhood.h"
#include "Place_List.h"
#include "Random.h"
#include "Household.h"
#include "School.h"


void insert_if_unique(place_vector* vec, Place* p) {
  for (place_vector::iterator itr = vec->begin(); itr != vec->end(); ++itr) {
    if(*itr == p) {
      return;
    }
  }
  vec->push_back(p);
}

Neighborhood_Patch::Neighborhood_Patch() {
  this->grid = NULL;
  this->row = -1;
  this-> col = -1;
  this->min_x = 0.0;
  this->min_y = 0.0;
  this->max_x = 0.0;
  this->max_y = 0.0;
  this->center_y = 0.0;
  this->center_x = 0.0;
  this->popsize = 0;
  this->mean_household_income = 0.0;
  this->neighborhood = NULL;
  this->census_tract_fips = 0;
}

void Neighborhood_Patch::setup(Neighborhood_Layer* grd, int i, int j) {
  this->grid = grd;
  this->row = i;
  this-> col = j;
  double patch_size = grid->get_patch_size();
  double grid_min_x = grid->get_min_x();
  double grid_min_y = grid->get_min_y();
  this->min_x = grid_min_x + (col)*patch_size;
  this->min_y = grid_min_y + (row)*patch_size;
  this->max_x = grid_min_x + (col+1)*patch_size;
  this->max_y = grid_min_y + (row+1)*patch_size;
  this->center_y = (min_y+max_y)/2.0;
  this->center_x = (min_x+max_x)/2.0;
  this->households.clear();
  this->schools.clear();
  this->workplaces.clear();
  this->hospitals.clear();
  this->schools_attended_by_neighborhood_residents.clear();
  this->workplaces_attended_by_neighborhood_residents.clear();
  this->popsize = 0;
  this->mean_household_income = 0.0;
  this->neighborhood = NULL;
  this->vector_control_status = 0;
  this->census_tract_fips = 0;
}

void Neighborhood_Patch::make_neighborhood() {
  char label[80];
  sprintf(label, "N-%04d-%04d", this->row, this->col);
  fred::geo lat = Geo::get_latitude(this->center_y);
  fred::geo lon = Geo::get_longitude(this->center_x);
  this->neighborhood = Global::Places.add_place(label,
						Place::TYPE_NEIGHBORHOOD,
						Place::SUBTYPE_NONE,
						lon, lat,
						this->census_tract_fips);
}

void Neighborhood_Patch::add_household(Household* p) {
  this->households.push_back(p);
  if (this->census_tract_fips == 0) {
    this->census_tract_fips = p->get_census_tract_fips();
  }
  if (Global::Householdfp != NULL) {
    fprintf(Global::Householdfp,"%s %.8f %.8f %f %f house_id: %d row = %d  col = %d  house_number = %d\n",
	    p->get_label(),p->get_longitude(),p->get_latitude(),
	    p->get_x(),p->get_y(), p->get_id(), row, col, get_number_of_households());
    fflush(Global::Householdfp);
  }
}

void Neighborhood_Patch::record_daily_activity_locations() {
  Household* house;
  Person* per;
  Place* p;
  School* s;

  // create lists of persons, workplaces, schools (by age)
  this->person.clear();
  this->schools_attended_by_neighborhood_residents.clear();
  this->workplaces_attended_by_neighborhood_residents.clear();
  for(int age = 0; age < Global::ADULT_AGE; age++) {
    this->schools_attended_by_neighborhood_residents_by_age[age].clear();
  }

  // char filename[FRED_STRING_SIZE];
  // sprintf(filename, "PATCHES/Neighborhood_Patch-%d-%d-households", row, col);
  // fp = fopen(filename, "w");
  int houses = get_number_of_households();
  for(int i = 0; i < houses; ++i) {
    // printf("house %d of %d\n", i, houses); fflush(stdout);
    house = static_cast<Household*>(households[i]);
    house->record_profile();
    this->mean_household_income += house->get_household_income();
    int hsize = house->get_size();
    // fprintf(fp, "%d ", hsize);
    for(int j = 0; j < hsize; ++j) {
      per = house->get_enrollee(j);
      person.push_back(per);
      p = per->get_activities()->get_workplace();
      if (p != NULL) {
	insert_if_unique(&workplaces_attended_by_neighborhood_residents,p);
      }
      s = static_cast<School *>(per->get_activities()->get_school());
      if(s != NULL) {
	insert_if_unique(&this->schools_attended_by_neighborhood_residents,s);
	for(int age = 0; age < Global::ADULT_AGE; ++age) {
	  if(s->get_students_in_grade(age) > 0) {
	    // school_by_age[age].push_back(s);
	    insert_if_unique(&schools_attended_by_neighborhood_residents_by_age[age],s);
          }
	}
      }
    }
    // fprintf(fp, "\n");
  }
  // fclose(fp);
  this->popsize = static_cast<int>(this->person.size());
  int households = get_number_of_households();
  if(households > 0) {
    this->mean_household_income /= households;
  }
}

Person* Neighborhood_Patch::select_random_person() {
  if(static_cast<int>(this->person.size()) == 0) {
    return NULL;
  }
  int i = Random::draw_random_int(0, (static_cast<int>(this->person.size())) - 1);
  return this->person[i];
}


Place* Neighborhood_Patch::select_random_household() {
  if(static_cast<int>(this->households.size()) == 0) {
    return NULL;
  }
  int i = Random::draw_random_int(0, (static_cast<int>(this->households.size())) - 1);
  return this->households[i];
}


void Neighborhood_Patch::quality_control() {
  if(Global::Quality_control > 1 && this->person.size() > 0) {
    fprintf(Global::Statusfp,
	    "PATCH row = %d col = %d  pop = %d  houses = %d work = %d schools = %d by_age ",
	    this->row, this->col, static_cast<int>(this->person.size()), static_cast<int>(this->households.size()), static_cast<int>(this->workplaces.size()), static_cast<int>(this->schools.size()));
    for(int age = 0; age < 20; ++age) {
      fprintf(Global::Statusfp, "%d ", static_cast<int>(this->schools_attended_by_neighborhood_residents_by_age[age].size()));
    }
    fprintf(Global::Statusfp, "\n");
    if(Global::Verbose > 0) {
      for(int i = 0; i < this->schools_attended_by_neighborhood_residents.size(); ++i) {
	School* s = static_cast<School*>(this->schools_attended_by_neighborhood_residents[i]);
	fprintf(Global::Statusfp, "School %d: %s by_age: ", i, s->get_label());
	for(int a = 0; a < 19; ++a) {
	  fprintf(Global::Statusfp, " %d:%d,%d ", a, s->get_students_in_grade(a), s->get_orig_students_in_grade(a));
	}
	fprintf(Global::Statusfp, "\n");
      }
      fflush(Global::Statusfp);
    }
  }
}


double Neighborhood_Patch::distance_to_patch(Neighborhood_Patch* p2) {
  double x1 = this->center_x;
  double y1 = this->center_y;
  double x2 = p2->get_center_x();
  double y2 = p2->get_center_y();
  return sqrt((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2));
}

void Neighborhood_Patch::register_place(Place* place) {
  if(place->is_school()) {
    FRED_VERBOSE(1, "register place: added school %s\n", place->get_label());
    this->schools.push_back(place);
  }
  if(place->is_workplace()) {
    FRED_VERBOSE(1, "register place: added workplace %s\n", place->get_label());
    this->workplaces.push_back(place);
  }
  if(place->is_hospital()) {
    FRED_VERBOSE(1, "register place: added hospital %s\n", place->get_label());
    this->hospitals.push_back(place);
  }
}
