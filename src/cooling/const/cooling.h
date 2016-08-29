/*******************************************************************************
 * This file is part of SWIFT.
 * Copyright (c) 2016 Matthieu Schaller (matthieu.schaller@durham.ac.uk)
 *                    Richard Bower (r.g.bower@durham.ac.uk)
 *                    Stefan Arridge  (stefan.arridge@durham.ac.uk)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/
#ifndef SWIFT_CONST_COOLING_H
#define SWIFT_CONST_COOLING_H

/**
 * @file src/cooling/const/cooling.h
 * @brief Routines related to the "constant cooling" cooling function.
 *
 * This is the simplest possible cooling function. A constant cooling rate with
 * a minimal energy floor is applied.
 */

/* Some standard headers. */
#include <math.h>

/* Local includes. */
#include "const.h"
#include "error.h"
#include "hydro.h"
#include "parser.h"
#include "part.h"
#include "physical_constants.h"
#include "units.h"

/**
 * @brief Properties of the cooling function.
 */
struct cooling_data {

  /*! Cooling rate in internal units */
  float lambda;

  /*! Minimally allowed internal energy of the particles */
  float min_energy;

  /*! Constant multiplication factor for time-step criterion */
  float cooling_tstep_mult;
};

/**
 * @brief Apply the cooling function to a particle.
 *
 * @param phys_const The physical constants in internal units.
 * @param us The internal system of units.
 * @param cooling The #cooling_data used in the run.
 * @param p Pointer to the particle data.
 * @param dt The time-step of this particle.
 */
__attribute__((always_inline)) INLINE static void cooling_cool_part(
    const struct phys_const* const phys_const, const struct UnitSystem* us,
    const struct cooling_data* cooling, struct part* p, float dt) {

  /* Get current internal energy (dt=0) */
  const float u_old = hydro_get_internal_energy(p, 0.f);

  /* Get cooling function properties */
  const float du_dt = -cooling->lambda;
  const float u_floor = cooling->min_energy;

  /* Constant cooling with a minimal floor */
  float u_new;
  if (u_old - du_dt * dt > u_floor) {
    u_new = u_old + du_dt * dt;
  } else {
    u_new = u_floor;
  }

  /* Update the internal energy */
  hydro_set_internal_energy(p, u_new);
}

/**
 * @brief Computes the cooling time-step.
 *
 * @param cooling The #cooling_data used in the run.
 * @param phys_const The physical constants in internal units.
 * @param p Pointer to the particle data.
 */
__attribute__((always_inline)) INLINE static double cooling_timestep(
    const struct cooling_data* cooling,
    const struct phys_const* const phys_const, const struct part* const p) {

  const float cooling_rate = cooling->lambda;
  const float internal_energy =
      hydro_get_internal_energy(p, 0);  // dt = 0 because using current entropy
  return cooling->cooling_tstep_mult * internal_energy / cooling_rate;
}

/**
 * @brief Initialises the cooling properties.
 *
 * @param parameter_file The parsed parameter file.
 * @param us The current internal system of units.
 * @param phys_const The physical constants in internal units.
 * @param cooling The cooling properties to initialize
 */
INLINE void cooling_init(const struct swift_params* parameter_file,
                         const struct UnitSystem* us,
                         const struct phys_const* phys_const,
                         struct cooling_data* cooling) {

  cooling->lambda = parser_get_param_double(parameter_file, "Cooling:lambda");
  cooling->min_energy =
      parser_get_param_double(parameter_file, "Cooling:min_energy");
  cooling->cooling_tstep_mult =
      parser_get_param_double(parameter_file, "Cooling:cooling_tstep_mult");
}

/**
 * @brief Prints the properties of the cooling model to stdout.
 *
 * @param cooling The properties of the cooling function.
 */
INLINE void cooling_print(const struct cooling_data* cooling) {

  message("Cooling function is 'Constant cooling' with rate %f and floor %f",
          cooling->lambda, cooling->min_energy);
}

#endif /* SWIFT_CONST_COOLING_H */
