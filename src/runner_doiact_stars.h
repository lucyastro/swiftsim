/*******************************************************************************
 * This file is part of SWIFT.
 * Copyright (c) 2016 Matthieu Schaller (matthieu.schaller@durham.ac.uk)
 *               2018 Loic Hausammann (loic.hausammann@epfl.ch)
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

#include "swift.h"

/**
 * @brief Calculate the number density of #part around the #spart
 *
 * @param r runner task
 * @param c cell
 * @param timer 1 if the time is to be recorded.
 */
void runner_doself_stars_density(struct runner *r, struct cell *c, int timer) {
  const struct engine *e = r->e;
  const struct cosmology *cosmo = e->cosmology;

  TIMER_TIC;

  /* Anything to do here? */
  if (!cell_is_active_stars(c, e)) return;

  /* Cosmological terms */
  const float a = cosmo->a;
  const float H = cosmo->H;

  const int scount = c->scount;
  const int count = c->count;
  struct spart *restrict sparts = c->sparts;
  struct part *restrict parts = c->parts;

  /* Loop over the sparts in ci. */
  for (int sid = 0; sid < scount; sid++) {

    /* Get a hold of the ith spart in ci. */
    struct spart *restrict si = &sparts[sid];
    const float hi = si->h;
    const float hig2 = hi * hi * kernel_gamma2;
    const float six[3] = {(float)(si->x[0] - c->loc[0]),
                          (float)(si->x[1] - c->loc[1]),
                          (float)(si->x[2] - c->loc[2])};

    /* Loop over the parts in cj. */
    for (int pjd = 0; pjd < count; pjd++) {

      /* Get a pointer to the jth particle. */
      struct part *restrict pj = &parts[pjd];
      const float hj = pj->h;

      /* Compute the pairwise distance. */
      const float pjx[3] = {(float)(pj->x[0] - c->loc[0]),
                            (float)(pj->x[1] - c->loc[1]),
                            (float)(pj->x[2] - c->loc[2])};
      float dx[3] = {six[0] - pjx[0], six[1] - pjx[1], six[2] - pjx[2]};
      const float r2 = dx[0] * dx[0] + dx[1] * dx[1] + dx[2] * dx[2];

#ifdef SWIFT_DEBUG_CHECKS
      /* Check that particles have been drifted to the current time */
      if (pj->ti_drift != e->ti_current)
        error("Particle pj not drifted to current time");
#endif

      if (r2 > 0.f && r2 < hig2) {
        runner_iact_nonsym_stars_density(r2, dx, hi, hj, si, pj, a, H);
      }
    } /* loop over the parts in ci. */
  }   /* loop over the sparts in ci. */

  TIMER_TOC(timer_doself_stars_density);
}

/**
 * @brief Calculate the number density of cj #part around the ci #spart
 *
 * @param r runner task
 * @param c cell
 * @param timer 1 if the time is to be recorded.
 */
void runner_dosubpair_stars_density(struct runner *r, struct cell *restrict ci,
                                    struct cell *restrict cj) {

  const struct engine *e = r->e;
  const struct cosmology *cosmo = e->cosmology;

  /* Anything to do here? */
  if (!cell_is_active_stars(ci, e) && !cell_is_active_stars(cj, e)) return;

  const int scount_i = ci->scount;
  const int count_j = cj->count;
  struct spart *restrict sparts_i = ci->sparts;
  struct part *restrict parts_j = cj->parts;

  /* Cosmological terms */
  const float a = cosmo->a;
  const float H = cosmo->H;

  /* Get the relative distance between the pairs, wrapping. */
  double shift[3] = {0.0, 0.0, 0.0};
  for (int k = 0; k < 3; k++) {
    if (cj->loc[k] - ci->loc[k] < -e->s->dim[k] / 2)
      shift[k] = e->s->dim[k];
    else if (cj->loc[k] - ci->loc[k] > e->s->dim[k] / 2)
      shift[k] = -e->s->dim[k];
  }

  /* Loop over the sparts in ci. */
  for (int sid = 0; sid < scount_i; sid++) {

    /* Get a hold of the ith spart in ci. */
    struct spart *restrict si = &sparts_i[sid];
    const float hi = si->h;
    const float hig2 = hi * hi * kernel_gamma2;
    const float six[3] = {(float)(si->x[0] - (cj->loc[0] + shift[0])),
                          (float)(si->x[1] - (cj->loc[1] + shift[1])),
                          (float)(si->x[2] - (cj->loc[2] + shift[2]))};

    /* Loop over the parts in cj. */
    for (int pjd = 0; pjd < count_j; pjd++) {

      /* Get a pointer to the jth particle. */
      struct part *restrict pj = &parts_j[pjd];
      const float hj = pj->h;

      /* Compute the pairwise distance. */
      const float pjx[3] = {(float)(pj->x[0] - cj->loc[0]),
                            (float)(pj->x[1] - cj->loc[1]),
                            (float)(pj->x[2] - cj->loc[2])};
      float dx[3] = {six[0] - pjx[0], six[1] - pjx[1], six[2] - pjx[2]};
      const float r2 = dx[0] * dx[0] + dx[1] * dx[1] + dx[2] * dx[2];

#ifdef SWIFT_DEBUG_CHECKS
      /* Check that particles have been drifted to the current time */
      if (pj->ti_drift != e->ti_current)
        error("Particle pj not drifted to current time");
#endif

      if (r2 < hig2)
        runner_iact_nonsym_stars_density(r2, dx, hi, hj, si, pj, a, H);

    } /* loop over the parts in cj. */
  }   /* loop over the parts in ci. */
}

void runner_dopair_stars_density(struct runner *r, struct cell *restrict ci,
                                 struct cell *restrict cj, int timer) {

  TIMER_TIC;

  runner_dosubpair_stars_density(r, ci, cj);
  runner_dosubpair_stars_density(r, cj, ci);

  if (timer) TIMER_TOC(timer_dopair_stars_density);
}

/**
 * @brief Compute the interactions between a cell pair, but only for the
 *      given indices in ci.
 *
 * Version using a brute-force algorithm.
 *
 * @param r The #runner.
 * @param ci The first #cell.
 * @param sparts_i The #part to interact with @c cj.
 * @param ind The list of indices of particles in @c ci to interact with.
 * @param scount The number of particles in @c ind.
 * @param cj The second #cell.
 * @param shift The shift vector to apply to the particles in ci.
 */
void runner_dopair_subset_stars_density(struct runner *r,
                                        struct cell *restrict ci,
                                        struct spart *restrict sparts_i,
                                        int *restrict ind, int scount,
                                        struct cell *restrict cj,
                                        const double *shift) {

  const struct engine *e = r->e;
  const struct cosmology *cosmo = e->cosmology;

  TIMER_TIC;

  const int count_j = cj->count;
  struct part *restrict parts_j = cj->parts;

  /* Cosmological terms */
  const float a = cosmo->a;
  const float H = cosmo->H;

  /* Loop over the parts_i. */
  for (int pid = 0; pid < scount; pid++) {

    /* Get a hold of the ith part in ci. */
    struct spart *restrict spi = &sparts_i[ind[pid]];
    double spix[3];
    for (int k = 0; k < 3; k++) spix[k] = spi->x[k] - shift[k];
    const float hi = spi->h;
    const float hig2 = hi * hi * kernel_gamma2;

#ifdef SWIFT_DEBUG_CHECKS
    if (!spart_is_active(spi, e))
      error("Trying to correct smoothing length of inactive particle !");
#endif

    /* Loop over the parts in cj. */
    for (int pjd = 0; pjd < count_j; pjd++) {

      /* Get a pointer to the jth particle. */
      struct part *restrict pj = &parts_j[pjd];

      /* Compute the pairwise distance. */
      float r2 = 0.0f;
      float dx[3];
      for (int k = 0; k < 3; k++) {
        dx[k] = spix[k] - pj->x[k];
        r2 += dx[k] * dx[k];
      }

#ifdef SWIFT_DEBUG_CHECKS
      /* Check that particles have been drifted to the current time */
      if (pj->ti_drift != e->ti_current)
        error("Particle pj not drifted to current time");
#endif
      /* Hit or miss? */
      if (r2 < hig2) {
        runner_iact_nonsym_stars_density(r2, dx, hi, pj->h, spi, pj, a, H);
      }
    } /* loop over the parts in cj. */
  }   /* loop over the parts in ci. */

  TIMER_TOC(timer_dopair_subset_naive);
}

/**
 * @brief Compute the interactions between a cell pair, but only for the
 *      given indices in ci.
 *
 * @param r The #runner.
 * @param ci The first #cell.
 * @param sparts The #spart to interact.
 * @param ind The list of indices of particles in @c ci to interact with.
 * @param scount The number of particles in @c ind.
 */
void runner_doself_subset_stars_density(struct runner *r,
                                        struct cell *restrict ci,
                                        struct spart *restrict sparts,
                                        int *restrict ind, int scount) {

  const struct engine *e = r->e;
  const struct cosmology *cosmo = e->cosmology;

  TIMER_TIC;

  /* Cosmological terms */
  const float a = cosmo->a;
  const float H = cosmo->H;

  const int count_i = ci->count;
  struct part *restrict parts_j = ci->parts;

  /* Loop over the parts in ci. */
  for (int spid = 0; spid < scount; spid++) {

    /* Get a hold of the ith part in ci. */
    struct spart *spi = &sparts[ind[spid]];
    const float spix[3] = {(float)(spi->x[0] - ci->loc[0]),
                           (float)(spi->x[1] - ci->loc[1]),
                           (float)(spi->x[2] - ci->loc[2])};
    const float hi = spi->h;
    const float hig2 = hi * hi * kernel_gamma2;

#ifdef SWIFT_DEBUG_CHECKS
    if (!spart_is_active(spi, e))
      error("Inactive particle in subset function!");
#endif

    /* Loop over the parts in cj. */
    for (int pjd = 0; pjd < count_i; pjd++) {

      /* Get a pointer to the jth particle. */
      struct part *restrict pj = &parts_j[pjd];
      const float hj = pj->h;

      /* Compute the pairwise distance. */
      const float pjx[3] = {(float)(pj->x[0] - ci->loc[0]),
                            (float)(pj->x[1] - ci->loc[1]),
                            (float)(pj->x[2] - ci->loc[2])};
      float dx[3] = {spix[0] - pjx[0], spix[1] - pjx[1], spix[2] - pjx[2]};
      const float r2 = dx[0] * dx[0] + dx[1] * dx[1] + dx[2] * dx[2];

#ifdef SWIFT_DEBUG_CHECKS
      /* Check that particles have been drifted to the current time */
      if (pj->ti_drift != e->ti_current)
        error("Particle pj not drifted to current time");
#endif

      /* Hit or miss? */
      if (r2 > 0.f && r2 < hig2) {
        runner_iact_nonsym_stars_density(r2, dx, hi, hj, spi, pj, a, H);
      }
    } /* loop over the parts in cj. */
  }   /* loop over the parts in ci. */

  TIMER_TOC(timer_doself_subset_stars_density);
}

/**
* @brief Determine which version of DOSELF_SUBSET needs to be called depending
* on the optimisation level.

* @param r The #runner.
* @param ci The first #cell.
* @param parts The #spart to interact.
* @param ind The list of indices of particles in @c ci to interact with.
* @param scount The number of particles in @c ind.
*/
void runner_doself_subset_branch_stars_density(struct runner *r,
                                               struct cell *restrict ci,
                                               struct spart *restrict sparts,
                                               int *restrict ind, int scount) {

  runner_doself_subset_stars_density(r, ci, sparts, ind, scount);
}

/**
 * @brief Determine which version of DOPAIR_SUBSET needs to be called depending
 * on the
 * orientation of the cells or whether DOPAIR_SUBSET needs to be called at all.
 *
 * @param r The #runner.
 * @param ci The first #cell.
 * @param sparts_i The #spart to interact with @c cj.
 * @param ind The list of indices of particles in @c ci to interact with.
 * @param scount The number of particles in @c ind.
 * @param cj The second #cell.
 */
void runner_dopair_subset_branch_stars_density(struct runner *r,
                                               struct cell *restrict ci,
                                               struct spart *restrict sparts_i,
                                               int *restrict ind, int scount,
                                               struct cell *restrict cj) {

  const struct engine *e = r->e;

  /* Get the relative distance between the pairs, wrapping. */
  double shift[3] = {0.0, 0.0, 0.0};
  for (int k = 0; k < 3; k++) {
    if (cj->loc[k] - ci->loc[k] < -e->s->dim[k] / 2)
      shift[k] = e->s->dim[k];
    else if (cj->loc[k] - ci->loc[k] > e->s->dim[k] / 2)
      shift[k] = -e->s->dim[k];
  }

  runner_dopair_subset_stars_density(r, ci, sparts_i, ind, scount, cj, shift);
}

void runner_dosub_subset_stars_density(struct runner *r, struct cell *ci,
                                       struct spart *sparts, int *ind,
                                       int scount, struct cell *cj, int sid,
                                       int gettimer) {

  const struct engine *e = r->e;
  struct space *s = e->s;

  TIMER_TIC;

  /* Should we even bother? */
  if (!cell_is_active_stars(ci, e) &&
      (cj == NULL || !cell_is_active_stars(cj, e)))
    return;
  if (ci->scount == 0 || (cj != NULL && cj->scount == 0)) return;

  /* Find out in which sub-cell of ci the parts are. */
  struct cell *sub = NULL;
  if (ci->split) {
    for (int k = 0; k < 8; k++) {
      if (ci->progeny[k] != NULL) {
        if (&sparts[ind[0]] >= &ci->progeny[k]->sparts[0] &&
            &sparts[ind[0]] < &ci->progeny[k]->sparts[ci->progeny[k]->scount]) {
          sub = ci->progeny[k];
          break;
        }
      }
    }
  }

  /* Is this a single cell? */
  if (cj == NULL) {

    /* Recurse? */
    if (cell_can_recurse_in_self_stars_task(ci)) {

      /* Loop over all progeny. */
      runner_dosub_subset_stars_density(r, sub, sparts, ind, scount, NULL, -1,
                                        0);
      for (int j = 0; j < 8; j++)
        if (ci->progeny[j] != sub && ci->progeny[j] != NULL)
          runner_dosub_subset_stars_density(r, sub, sparts, ind, scount,
                                            ci->progeny[j], -1, 0);

    }

    /* Otherwise, compute self-interaction. */
    else
      runner_doself_subset_branch_stars_density(r, ci, sparts, ind, scount);
  } /* self-interaction. */

  /* Otherwise, it's a pair interaction. */
  else {

    /* Recurse? */
    if (cell_can_recurse_in_pair_stars_task(ci) &&
        cell_can_recurse_in_pair_stars_task(cj)) {

      /* Get the type of pair if not specified explicitly. */
      double shift[3] = {0.0, 0.0, 0.0};
      sid = space_getsid(s, &ci, &cj, shift);

      /* Different types of flags. */
      switch (sid) {

        /* Regular sub-cell interactions of a single cell. */
        case 0: /* (  1 ,  1 ,  1 ) */
          if (ci->progeny[7] == sub && cj->progeny[0] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[7], sparts, ind,
                                              scount, cj->progeny[0], -1, 0);
          if (ci->progeny[7] != NULL && cj->progeny[0] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[0], sparts, ind,
                                              scount, ci->progeny[7], -1, 0);
          break;

        case 1: /* (  1 ,  1 ,  0 ) */
          if (ci->progeny[6] == sub && cj->progeny[0] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[6], sparts, ind,
                                              scount, cj->progeny[0], -1, 0);
          if (ci->progeny[6] != NULL && cj->progeny[0] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[0], sparts, ind,
                                              scount, ci->progeny[6], -1, 0);
          if (ci->progeny[6] == sub && cj->progeny[1] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[6], sparts, ind,
                                              scount, cj->progeny[1], -1, 0);
          if (ci->progeny[6] != NULL && cj->progeny[1] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[1], sparts, ind,
                                              scount, ci->progeny[6], -1, 0);
          if (ci->progeny[7] == sub && cj->progeny[0] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[7], sparts, ind,
                                              scount, cj->progeny[0], -1, 0);
          if (ci->progeny[7] != NULL && cj->progeny[0] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[0], sparts, ind,
                                              scount, ci->progeny[7], -1, 0);
          if (ci->progeny[7] == sub && cj->progeny[1] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[7], sparts, ind,
                                              scount, cj->progeny[1], -1, 0);
          if (ci->progeny[7] != NULL && cj->progeny[1] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[1], sparts, ind,
                                              scount, ci->progeny[7], -1, 0);
          break;

        case 2: /* (  1 ,  1 , -1 ) */
          if (ci->progeny[6] == sub && cj->progeny[1] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[6], sparts, ind,
                                              scount, cj->progeny[1], -1, 0);
          if (ci->progeny[6] != NULL && cj->progeny[1] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[1], sparts, ind,
                                              scount, ci->progeny[6], -1, 0);
          break;

        case 3: /* (  1 ,  0 ,  1 ) */
          if (ci->progeny[5] == sub && cj->progeny[0] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[5], sparts, ind,
                                              scount, cj->progeny[0], -1, 0);
          if (ci->progeny[5] != NULL && cj->progeny[0] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[0], sparts, ind,
                                              scount, ci->progeny[5], -1, 0);
          if (ci->progeny[5] == sub && cj->progeny[2] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[5], sparts, ind,
                                              scount, cj->progeny[2], -1, 0);
          if (ci->progeny[5] != NULL && cj->progeny[2] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[2], sparts, ind,
                                              scount, ci->progeny[5], -1, 0);
          if (ci->progeny[7] == sub && cj->progeny[0] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[7], sparts, ind,
                                              scount, cj->progeny[0], -1, 0);
          if (ci->progeny[7] != NULL && cj->progeny[0] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[0], sparts, ind,
                                              scount, ci->progeny[7], -1, 0);
          if (ci->progeny[7] == sub && cj->progeny[2] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[7], sparts, ind,
                                              scount, cj->progeny[2], -1, 0);
          if (ci->progeny[7] != NULL && cj->progeny[2] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[2], sparts, ind,
                                              scount, ci->progeny[7], -1, 0);
          break;

        case 4: /* (  1 ,  0 ,  0 ) */
          if (ci->progeny[4] == sub && cj->progeny[0] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[4], sparts, ind,
                                              scount, cj->progeny[0], -1, 0);
          if (ci->progeny[4] != NULL && cj->progeny[0] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[0], sparts, ind,
                                              scount, ci->progeny[4], -1, 0);
          if (ci->progeny[4] == sub && cj->progeny[1] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[4], sparts, ind,
                                              scount, cj->progeny[1], -1, 0);
          if (ci->progeny[4] != NULL && cj->progeny[1] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[1], sparts, ind,
                                              scount, ci->progeny[4], -1, 0);
          if (ci->progeny[4] == sub && cj->progeny[2] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[4], sparts, ind,
                                              scount, cj->progeny[2], -1, 0);
          if (ci->progeny[4] != NULL && cj->progeny[2] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[2], sparts, ind,
                                              scount, ci->progeny[4], -1, 0);
          if (ci->progeny[4] == sub && cj->progeny[3] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[4], sparts, ind,
                                              scount, cj->progeny[3], -1, 0);
          if (ci->progeny[4] != NULL && cj->progeny[3] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[3], sparts, ind,
                                              scount, ci->progeny[4], -1, 0);
          if (ci->progeny[5] == sub && cj->progeny[0] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[5], sparts, ind,
                                              scount, cj->progeny[0], -1, 0);
          if (ci->progeny[5] != NULL && cj->progeny[0] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[0], sparts, ind,
                                              scount, ci->progeny[5], -1, 0);
          if (ci->progeny[5] == sub && cj->progeny[1] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[5], sparts, ind,
                                              scount, cj->progeny[1], -1, 0);
          if (ci->progeny[5] != NULL && cj->progeny[1] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[1], sparts, ind,
                                              scount, ci->progeny[5], -1, 0);
          if (ci->progeny[5] == sub && cj->progeny[2] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[5], sparts, ind,
                                              scount, cj->progeny[2], -1, 0);
          if (ci->progeny[5] != NULL && cj->progeny[2] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[2], sparts, ind,
                                              scount, ci->progeny[5], -1, 0);
          if (ci->progeny[5] == sub && cj->progeny[3] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[5], sparts, ind,
                                              scount, cj->progeny[3], -1, 0);
          if (ci->progeny[5] != NULL && cj->progeny[3] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[3], sparts, ind,
                                              scount, ci->progeny[5], -1, 0);
          if (ci->progeny[6] == sub && cj->progeny[0] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[6], sparts, ind,
                                              scount, cj->progeny[0], -1, 0);
          if (ci->progeny[6] != NULL && cj->progeny[0] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[0], sparts, ind,
                                              scount, ci->progeny[6], -1, 0);
          if (ci->progeny[6] == sub && cj->progeny[1] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[6], sparts, ind,
                                              scount, cj->progeny[1], -1, 0);
          if (ci->progeny[6] != NULL && cj->progeny[1] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[1], sparts, ind,
                                              scount, ci->progeny[6], -1, 0);
          if (ci->progeny[6] == sub && cj->progeny[2] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[6], sparts, ind,
                                              scount, cj->progeny[2], -1, 0);
          if (ci->progeny[6] != NULL && cj->progeny[2] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[2], sparts, ind,
                                              scount, ci->progeny[6], -1, 0);
          if (ci->progeny[6] == sub && cj->progeny[3] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[6], sparts, ind,
                                              scount, cj->progeny[3], -1, 0);
          if (ci->progeny[6] != NULL && cj->progeny[3] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[3], sparts, ind,
                                              scount, ci->progeny[6], -1, 0);
          if (ci->progeny[7] == sub && cj->progeny[0] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[7], sparts, ind,
                                              scount, cj->progeny[0], -1, 0);
          if (ci->progeny[7] != NULL && cj->progeny[0] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[0], sparts, ind,
                                              scount, ci->progeny[7], -1, 0);
          if (ci->progeny[7] == sub && cj->progeny[1] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[7], sparts, ind,
                                              scount, cj->progeny[1], -1, 0);
          if (ci->progeny[7] != NULL && cj->progeny[1] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[1], sparts, ind,
                                              scount, ci->progeny[7], -1, 0);
          if (ci->progeny[7] == sub && cj->progeny[2] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[7], sparts, ind,
                                              scount, cj->progeny[2], -1, 0);
          if (ci->progeny[7] != NULL && cj->progeny[2] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[2], sparts, ind,
                                              scount, ci->progeny[7], -1, 0);
          if (ci->progeny[7] == sub && cj->progeny[3] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[7], sparts, ind,
                                              scount, cj->progeny[3], -1, 0);
          if (ci->progeny[7] != NULL && cj->progeny[3] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[3], sparts, ind,
                                              scount, ci->progeny[7], -1, 0);
          break;

        case 5: /* (  1 ,  0 , -1 ) */
          if (ci->progeny[4] == sub && cj->progeny[1] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[4], sparts, ind,
                                              scount, cj->progeny[1], -1, 0);
          if (ci->progeny[4] != NULL && cj->progeny[1] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[1], sparts, ind,
                                              scount, ci->progeny[4], -1, 0);
          if (ci->progeny[4] == sub && cj->progeny[3] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[4], sparts, ind,
                                              scount, cj->progeny[3], -1, 0);
          if (ci->progeny[4] != NULL && cj->progeny[3] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[3], sparts, ind,
                                              scount, ci->progeny[4], -1, 0);
          if (ci->progeny[6] == sub && cj->progeny[1] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[6], sparts, ind,
                                              scount, cj->progeny[1], -1, 0);
          if (ci->progeny[6] != NULL && cj->progeny[1] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[1], sparts, ind,
                                              scount, ci->progeny[6], -1, 0);
          if (ci->progeny[6] == sub && cj->progeny[3] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[6], sparts, ind,
                                              scount, cj->progeny[3], -1, 0);
          if (ci->progeny[6] != NULL && cj->progeny[3] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[3], sparts, ind,
                                              scount, ci->progeny[6], -1, 0);
          break;

        case 6: /* (  1 , -1 ,  1 ) */
          if (ci->progeny[5] == sub && cj->progeny[2] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[5], sparts, ind,
                                              scount, cj->progeny[2], -1, 0);
          if (ci->progeny[5] != NULL && cj->progeny[2] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[2], sparts, ind,
                                              scount, ci->progeny[5], -1, 0);
          break;

        case 7: /* (  1 , -1 ,  0 ) */
          if (ci->progeny[4] == sub && cj->progeny[2] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[4], sparts, ind,
                                              scount, cj->progeny[2], -1, 0);
          if (ci->progeny[4] != NULL && cj->progeny[2] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[2], sparts, ind,
                                              scount, ci->progeny[4], -1, 0);
          if (ci->progeny[4] == sub && cj->progeny[3] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[4], sparts, ind,
                                              scount, cj->progeny[3], -1, 0);
          if (ci->progeny[4] != NULL && cj->progeny[3] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[3], sparts, ind,
                                              scount, ci->progeny[4], -1, 0);
          if (ci->progeny[5] == sub && cj->progeny[2] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[5], sparts, ind,
                                              scount, cj->progeny[2], -1, 0);
          if (ci->progeny[5] != NULL && cj->progeny[2] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[2], sparts, ind,
                                              scount, ci->progeny[5], -1, 0);
          if (ci->progeny[5] == sub && cj->progeny[3] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[5], sparts, ind,
                                              scount, cj->progeny[3], -1, 0);
          if (ci->progeny[5] != NULL && cj->progeny[3] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[3], sparts, ind,
                                              scount, ci->progeny[5], -1, 0);
          break;

        case 8: /* (  1 , -1 , -1 ) */
          if (ci->progeny[4] == sub && cj->progeny[3] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[4], sparts, ind,
                                              scount, cj->progeny[3], -1, 0);
          if (ci->progeny[4] != NULL && cj->progeny[3] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[3], sparts, ind,
                                              scount, ci->progeny[4], -1, 0);
          break;

        case 9: /* (  0 ,  1 ,  1 ) */
          if (ci->progeny[3] == sub && cj->progeny[0] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[3], sparts, ind,
                                              scount, cj->progeny[0], -1, 0);
          if (ci->progeny[3] != NULL && cj->progeny[0] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[0], sparts, ind,
                                              scount, ci->progeny[3], -1, 0);
          if (ci->progeny[3] == sub && cj->progeny[4] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[3], sparts, ind,
                                              scount, cj->progeny[4], -1, 0);
          if (ci->progeny[3] != NULL && cj->progeny[4] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[4], sparts, ind,
                                              scount, ci->progeny[3], -1, 0);
          if (ci->progeny[7] == sub && cj->progeny[0] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[7], sparts, ind,
                                              scount, cj->progeny[0], -1, 0);
          if (ci->progeny[7] != NULL && cj->progeny[0] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[0], sparts, ind,
                                              scount, ci->progeny[7], -1, 0);
          if (ci->progeny[7] == sub && cj->progeny[4] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[7], sparts, ind,
                                              scount, cj->progeny[4], -1, 0);
          if (ci->progeny[7] != NULL && cj->progeny[4] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[4], sparts, ind,
                                              scount, ci->progeny[7], -1, 0);
          break;

        case 10: /* (  0 ,  1 ,  0 ) */
          if (ci->progeny[2] == sub && cj->progeny[0] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[2], sparts, ind,
                                              scount, cj->progeny[0], -1, 0);
          if (ci->progeny[2] != NULL && cj->progeny[0] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[0], sparts, ind,
                                              scount, ci->progeny[2], -1, 0);
          if (ci->progeny[2] == sub && cj->progeny[1] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[2], sparts, ind,
                                              scount, cj->progeny[1], -1, 0);
          if (ci->progeny[2] != NULL && cj->progeny[1] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[1], sparts, ind,
                                              scount, ci->progeny[2], -1, 0);
          if (ci->progeny[2] == sub && cj->progeny[4] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[2], sparts, ind,
                                              scount, cj->progeny[4], -1, 0);
          if (ci->progeny[2] != NULL && cj->progeny[4] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[4], sparts, ind,
                                              scount, ci->progeny[2], -1, 0);
          if (ci->progeny[2] == sub && cj->progeny[5] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[2], sparts, ind,
                                              scount, cj->progeny[5], -1, 0);
          if (ci->progeny[2] != NULL && cj->progeny[5] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[5], sparts, ind,
                                              scount, ci->progeny[2], -1, 0);
          if (ci->progeny[3] == sub && cj->progeny[0] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[3], sparts, ind,
                                              scount, cj->progeny[0], -1, 0);
          if (ci->progeny[3] != NULL && cj->progeny[0] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[0], sparts, ind,
                                              scount, ci->progeny[3], -1, 0);
          if (ci->progeny[3] == sub && cj->progeny[1] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[3], sparts, ind,
                                              scount, cj->progeny[1], -1, 0);
          if (ci->progeny[3] != NULL && cj->progeny[1] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[1], sparts, ind,
                                              scount, ci->progeny[3], -1, 0);
          if (ci->progeny[3] == sub && cj->progeny[4] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[3], sparts, ind,
                                              scount, cj->progeny[4], -1, 0);
          if (ci->progeny[3] != NULL && cj->progeny[4] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[4], sparts, ind,
                                              scount, ci->progeny[3], -1, 0);
          if (ci->progeny[3] == sub && cj->progeny[5] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[3], sparts, ind,
                                              scount, cj->progeny[5], -1, 0);
          if (ci->progeny[3] != NULL && cj->progeny[5] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[5], sparts, ind,
                                              scount, ci->progeny[3], -1, 0);
          if (ci->progeny[6] == sub && cj->progeny[0] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[6], sparts, ind,
                                              scount, cj->progeny[0], -1, 0);
          if (ci->progeny[6] != NULL && cj->progeny[0] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[0], sparts, ind,
                                              scount, ci->progeny[6], -1, 0);
          if (ci->progeny[6] == sub && cj->progeny[1] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[6], sparts, ind,
                                              scount, cj->progeny[1], -1, 0);
          if (ci->progeny[6] != NULL && cj->progeny[1] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[1], sparts, ind,
                                              scount, ci->progeny[6], -1, 0);
          if (ci->progeny[6] == sub && cj->progeny[4] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[6], sparts, ind,
                                              scount, cj->progeny[4], -1, 0);
          if (ci->progeny[6] != NULL && cj->progeny[4] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[4], sparts, ind,
                                              scount, ci->progeny[6], -1, 0);
          if (ci->progeny[6] == sub && cj->progeny[5] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[6], sparts, ind,
                                              scount, cj->progeny[5], -1, 0);
          if (ci->progeny[6] != NULL && cj->progeny[5] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[5], sparts, ind,
                                              scount, ci->progeny[6], -1, 0);
          if (ci->progeny[7] == sub && cj->progeny[0] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[7], sparts, ind,
                                              scount, cj->progeny[0], -1, 0);
          if (ci->progeny[7] != NULL && cj->progeny[0] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[0], sparts, ind,
                                              scount, ci->progeny[7], -1, 0);
          if (ci->progeny[7] == sub && cj->progeny[1] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[7], sparts, ind,
                                              scount, cj->progeny[1], -1, 0);
          if (ci->progeny[7] != NULL && cj->progeny[1] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[1], sparts, ind,
                                              scount, ci->progeny[7], -1, 0);
          if (ci->progeny[7] == sub && cj->progeny[4] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[7], sparts, ind,
                                              scount, cj->progeny[4], -1, 0);
          if (ci->progeny[7] != NULL && cj->progeny[4] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[4], sparts, ind,
                                              scount, ci->progeny[7], -1, 0);
          if (ci->progeny[7] == sub && cj->progeny[5] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[7], sparts, ind,
                                              scount, cj->progeny[5], -1, 0);
          if (ci->progeny[7] != NULL && cj->progeny[5] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[5], sparts, ind,
                                              scount, ci->progeny[7], -1, 0);
          break;

        case 11: /* (  0 ,  1 , -1 ) */
          if (ci->progeny[2] == sub && cj->progeny[1] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[2], sparts, ind,
                                              scount, cj->progeny[1], -1, 0);
          if (ci->progeny[2] != NULL && cj->progeny[1] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[1], sparts, ind,
                                              scount, ci->progeny[2], -1, 0);
          if (ci->progeny[2] == sub && cj->progeny[5] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[2], sparts, ind,
                                              scount, cj->progeny[5], -1, 0);
          if (ci->progeny[2] != NULL && cj->progeny[5] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[5], sparts, ind,
                                              scount, ci->progeny[2], -1, 0);
          if (ci->progeny[6] == sub && cj->progeny[1] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[6], sparts, ind,
                                              scount, cj->progeny[1], -1, 0);
          if (ci->progeny[6] != NULL && cj->progeny[1] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[1], sparts, ind,
                                              scount, ci->progeny[6], -1, 0);
          if (ci->progeny[6] == sub && cj->progeny[5] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[6], sparts, ind,
                                              scount, cj->progeny[5], -1, 0);
          if (ci->progeny[6] != NULL && cj->progeny[5] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[5], sparts, ind,
                                              scount, ci->progeny[6], -1, 0);
          break;

        case 12: /* (  0 ,  0 ,  1 ) */
          if (ci->progeny[1] == sub && cj->progeny[0] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[1], sparts, ind,
                                              scount, cj->progeny[0], -1, 0);
          if (ci->progeny[1] != NULL && cj->progeny[0] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[0], sparts, ind,
                                              scount, ci->progeny[1], -1, 0);
          if (ci->progeny[1] == sub && cj->progeny[2] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[1], sparts, ind,
                                              scount, cj->progeny[2], -1, 0);
          if (ci->progeny[1] != NULL && cj->progeny[2] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[2], sparts, ind,
                                              scount, ci->progeny[1], -1, 0);
          if (ci->progeny[1] == sub && cj->progeny[4] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[1], sparts, ind,
                                              scount, cj->progeny[4], -1, 0);
          if (ci->progeny[1] != NULL && cj->progeny[4] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[4], sparts, ind,
                                              scount, ci->progeny[1], -1, 0);
          if (ci->progeny[1] == sub && cj->progeny[6] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[1], sparts, ind,
                                              scount, cj->progeny[6], -1, 0);
          if (ci->progeny[1] != NULL && cj->progeny[6] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[6], sparts, ind,
                                              scount, ci->progeny[1], -1, 0);
          if (ci->progeny[3] == sub && cj->progeny[0] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[3], sparts, ind,
                                              scount, cj->progeny[0], -1, 0);
          if (ci->progeny[3] != NULL && cj->progeny[0] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[0], sparts, ind,
                                              scount, ci->progeny[3], -1, 0);
          if (ci->progeny[3] == sub && cj->progeny[2] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[3], sparts, ind,
                                              scount, cj->progeny[2], -1, 0);
          if (ci->progeny[3] != NULL && cj->progeny[2] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[2], sparts, ind,
                                              scount, ci->progeny[3], -1, 0);
          if (ci->progeny[3] == sub && cj->progeny[4] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[3], sparts, ind,
                                              scount, cj->progeny[4], -1, 0);
          if (ci->progeny[3] != NULL && cj->progeny[4] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[4], sparts, ind,
                                              scount, ci->progeny[3], -1, 0);
          if (ci->progeny[3] == sub && cj->progeny[6] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[3], sparts, ind,
                                              scount, cj->progeny[6], -1, 0);
          if (ci->progeny[3] != NULL && cj->progeny[6] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[6], sparts, ind,
                                              scount, ci->progeny[3], -1, 0);
          if (ci->progeny[5] == sub && cj->progeny[0] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[5], sparts, ind,
                                              scount, cj->progeny[0], -1, 0);
          if (ci->progeny[5] != NULL && cj->progeny[0] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[0], sparts, ind,
                                              scount, ci->progeny[5], -1, 0);
          if (ci->progeny[5] == sub && cj->progeny[2] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[5], sparts, ind,
                                              scount, cj->progeny[2], -1, 0);
          if (ci->progeny[5] != NULL && cj->progeny[2] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[2], sparts, ind,
                                              scount, ci->progeny[5], -1, 0);
          if (ci->progeny[5] == sub && cj->progeny[4] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[5], sparts, ind,
                                              scount, cj->progeny[4], -1, 0);
          if (ci->progeny[5] != NULL && cj->progeny[4] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[4], sparts, ind,
                                              scount, ci->progeny[5], -1, 0);
          if (ci->progeny[5] == sub && cj->progeny[6] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[5], sparts, ind,
                                              scount, cj->progeny[6], -1, 0);
          if (ci->progeny[5] != NULL && cj->progeny[6] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[6], sparts, ind,
                                              scount, ci->progeny[5], -1, 0);
          if (ci->progeny[7] == sub && cj->progeny[0] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[7], sparts, ind,
                                              scount, cj->progeny[0], -1, 0);
          if (ci->progeny[7] != NULL && cj->progeny[0] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[0], sparts, ind,
                                              scount, ci->progeny[7], -1, 0);
          if (ci->progeny[7] == sub && cj->progeny[2] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[7], sparts, ind,
                                              scount, cj->progeny[2], -1, 0);
          if (ci->progeny[7] != NULL && cj->progeny[2] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[2], sparts, ind,
                                              scount, ci->progeny[7], -1, 0);
          if (ci->progeny[7] == sub && cj->progeny[4] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[7], sparts, ind,
                                              scount, cj->progeny[4], -1, 0);
          if (ci->progeny[7] != NULL && cj->progeny[4] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[4], sparts, ind,
                                              scount, ci->progeny[7], -1, 0);
          if (ci->progeny[7] == sub && cj->progeny[6] != NULL)
            runner_dosub_subset_stars_density(r, ci->progeny[7], sparts, ind,
                                              scount, cj->progeny[6], -1, 0);
          if (ci->progeny[7] != NULL && cj->progeny[6] == sub)
            runner_dosub_subset_stars_density(r, cj->progeny[6], sparts, ind,
                                              scount, ci->progeny[7], -1, 0);
          break;
      }

    }

    /* Otherwise, compute the pair directly. */
    else if (cell_is_active_stars(ci, e) || cell_is_active_stars(cj, e)) {

      /* Do any of the cells need to be drifted first? */
      if (!cell_are_part_drifted(cj, e)) error("Cell should be drifted!");

      runner_dopair_subset_branch_stars_density(r, ci, sparts, ind, scount, cj);
    }

  } /* otherwise, pair interaction. */

  if (gettimer) TIMER_TOC(timer_dosub_subset);
}

/**
 * @brief Determine which version of runner_doself needs to be called depending
 * on the optimisation level.
 *
 * @param r #runner
 * @param c #cell c
 *
 */
void runner_doself_branch_stars_density(struct runner *r, struct cell *c) {

  const struct engine *restrict e = r->e;

  /* Anything to do here? */
  if (!cell_is_active_stars(c, e)) return;

  /* Did we mess up the recursion? */
  if (c->h_max_old * kernel_gamma > c->dmin)
    error("Cell smaller than smoothing length");

  runner_doself_stars_density(r, c, 1);
}

/**
 * @brief Determine which version of DOPAIR1 needs to be called depending on the
 * orientation of the cells or whether DOPAIR1 needs to be called at all.
 *
 * @param r #runner
 * @param ci #cell ci
 * @param cj #cell cj
 *
 */
void runner_dopair_branch_stars_density(struct runner *r, struct cell *ci,
                                        struct cell *cj) {

  const struct engine *restrict e = r->e;

  /* Anything to do here? */
  if (!cell_is_active_stars(ci, e) && !cell_is_active_stars(cj, e)) return;

  /* Check that cells are drifted. */
  if (!cell_are_part_drifted(ci, e) || !cell_are_part_drifted(cj, e))
    error("Interacting undrifted cells.");

  /* Get the sort ID. */
  double shift[3] = {0.0, 0.0, 0.0};
  const int sid = space_getsid(e->s, &ci, &cj, shift);

  /* Have the cells been sorted? */
  if (!(ci->sorted & (1 << sid)) ||
      ci->dx_max_sort_old > space_maxreldx * ci->dmin)
    error("Interacting unsorted cells.");
  if (!(cj->sorted & (1 << sid)) ||
      cj->dx_max_sort_old > space_maxreldx * cj->dmin)
    error("Interacting unsorted cells.");

#ifdef SWIFT_DEBUG_CHECKS
  /* Pick-out the sorted lists. */
  const struct entry *restrict sort_i = ci->sort[sid];
  const struct entry *restrict sort_j = cj->sort[sid];

  /* Check that the dx_max_sort values in the cell are indeed an upper
     bound on particle movement. */
  for (int pid = 0; pid < ci->count; pid++) {
    const struct part *p = &ci->parts[sort_i[pid].i];
    const float d = p->x[0] * runner_shift[sid][0] +
                    p->x[1] * runner_shift[sid][1] +
                    p->x[2] * runner_shift[sid][2];
    if (fabsf(d - sort_i[pid].d) - ci->dx_max_sort >
            1.0e-4 * max(fabsf(d), ci->dx_max_sort_old) &&
        fabsf(d - sort_i[pid].d) - ci->dx_max_sort > ci->width[0] * 1.0e-10)
      error(
          "particle shift diff exceeds dx_max_sort in cell ci. ci->nodeID=%d "
          "cj->nodeID=%d d=%e sort_i[pid].d=%e ci->dx_max_sort=%e "
          "ci->dx_max_sort_old=%e",
          ci->nodeID, cj->nodeID, d, sort_i[pid].d, ci->dx_max_sort,
          ci->dx_max_sort_old);
  }
  for (int pjd = 0; pjd < cj->count; pjd++) {
    const struct part *p = &cj->parts[sort_j[pjd].i];
    const float d = p->x[0] * runner_shift[sid][0] +
                    p->x[1] * runner_shift[sid][1] +
                    p->x[2] * runner_shift[sid][2];
    if ((fabsf(d - sort_j[pjd].d) - cj->dx_max_sort) >
            1.0e-4 * max(fabsf(d), cj->dx_max_sort_old) &&
        (fabsf(d - sort_j[pjd].d) - cj->dx_max_sort) > cj->width[0] * 1.0e-10)
      error(
          "particle shift diff exceeds dx_max_sort in cell cj. cj->nodeID=%d "
          "ci->nodeID=%d d=%e sort_j[pjd].d=%e cj->dx_max_sort=%e "
          "cj->dx_max_sort_old=%e",
          cj->nodeID, ci->nodeID, d, sort_j[pjd].d, cj->dx_max_sort,
          cj->dx_max_sort_old);
  }
#endif /* SWIFT_DEBUG_CHECKS */

  runner_dopair_stars_density(r, ci, cj, 1);
}

/**
 * @brief Compute grouped sub-cell interactions for pairs
 *
 * @param r The #runner.
 * @param ci The first #cell.
 * @param cj The second #cell.
 * @param sid The direction linking the cells
 * @param gettimer Do we have a timer ?
 *
 * @todo Hard-code the sid on the recursive calls to avoid the
 * redundant computations to find the sid on-the-fly.
 */
void runner_dosub_pair_stars_density(struct runner *r, struct cell *ci,
                                     struct cell *cj, int sid, int gettimer) {

  struct space *s = r->e->s;
  const struct engine *e = r->e;

  TIMER_TIC;

  /* Should we even bother? */
  if (!cell_is_active_stars(ci, e) && !cell_is_active_stars(cj, e)) return;
  if (ci->scount == 0 || cj->scount == 0) return;

  /* Get the type of pair if not specified explicitly. */
  double shift[3];
  sid = space_getsid(s, &ci, &cj, shift);

  /* Recurse? */
  if (cell_can_recurse_in_pair_stars_task(ci) &&
      cell_can_recurse_in_pair_stars_task(cj)) {

    /* Different types of flags. */
    switch (sid) {

      /* Regular sub-cell interactions of a single cell. */
      case 0: /* (  1 ,  1 ,  1 ) */
        if (ci->progeny[7] != NULL && cj->progeny[0] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[7], cj->progeny[0], -1,
                                          0);
        break;

      case 1: /* (  1 ,  1 ,  0 ) */
        if (ci->progeny[6] != NULL && cj->progeny[0] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[6], cj->progeny[0], -1,
                                          0);
        if (ci->progeny[6] != NULL && cj->progeny[1] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[6], cj->progeny[1], -1,
                                          0);
        if (ci->progeny[7] != NULL && cj->progeny[0] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[7], cj->progeny[0], -1,
                                          0);
        if (ci->progeny[7] != NULL && cj->progeny[1] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[7], cj->progeny[1], -1,
                                          0);
        break;

      case 2: /* (  1 ,  1 , -1 ) */
        if (ci->progeny[6] != NULL && cj->progeny[1] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[6], cj->progeny[1], -1,
                                          0);
        break;

      case 3: /* (  1 ,  0 ,  1 ) */
        if (ci->progeny[5] != NULL && cj->progeny[0] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[5], cj->progeny[0], -1,
                                          0);
        if (ci->progeny[5] != NULL && cj->progeny[2] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[5], cj->progeny[2], -1,
                                          0);
        if (ci->progeny[7] != NULL && cj->progeny[0] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[7], cj->progeny[0], -1,
                                          0);
        if (ci->progeny[7] != NULL && cj->progeny[2] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[7], cj->progeny[2], -1,
                                          0);
        break;

      case 4: /* (  1 ,  0 ,  0 ) */
        if (ci->progeny[4] != NULL && cj->progeny[0] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[4], cj->progeny[0], -1,
                                          0);
        if (ci->progeny[4] != NULL && cj->progeny[1] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[4], cj->progeny[1], -1,
                                          0);
        if (ci->progeny[4] != NULL && cj->progeny[2] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[4], cj->progeny[2], -1,
                                          0);
        if (ci->progeny[4] != NULL && cj->progeny[3] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[4], cj->progeny[3], -1,
                                          0);
        if (ci->progeny[5] != NULL && cj->progeny[0] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[5], cj->progeny[0], -1,
                                          0);
        if (ci->progeny[5] != NULL && cj->progeny[1] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[5], cj->progeny[1], -1,
                                          0);
        if (ci->progeny[5] != NULL && cj->progeny[2] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[5], cj->progeny[2], -1,
                                          0);
        if (ci->progeny[5] != NULL && cj->progeny[3] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[5], cj->progeny[3], -1,
                                          0);
        if (ci->progeny[6] != NULL && cj->progeny[0] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[6], cj->progeny[0], -1,
                                          0);
        if (ci->progeny[6] != NULL && cj->progeny[1] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[6], cj->progeny[1], -1,
                                          0);
        if (ci->progeny[6] != NULL && cj->progeny[2] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[6], cj->progeny[2], -1,
                                          0);
        if (ci->progeny[6] != NULL && cj->progeny[3] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[6], cj->progeny[3], -1,
                                          0);
        if (ci->progeny[7] != NULL && cj->progeny[0] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[7], cj->progeny[0], -1,
                                          0);
        if (ci->progeny[7] != NULL && cj->progeny[1] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[7], cj->progeny[1], -1,
                                          0);
        if (ci->progeny[7] != NULL && cj->progeny[2] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[7], cj->progeny[2], -1,
                                          0);
        if (ci->progeny[7] != NULL && cj->progeny[3] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[7], cj->progeny[3], -1,
                                          0);
        break;

      case 5: /* (  1 ,  0 , -1 ) */
        if (ci->progeny[4] != NULL && cj->progeny[1] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[4], cj->progeny[1], -1,
                                          0);
        if (ci->progeny[4] != NULL && cj->progeny[3] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[4], cj->progeny[3], -1,
                                          0);
        if (ci->progeny[6] != NULL && cj->progeny[1] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[6], cj->progeny[1], -1,
                                          0);
        if (ci->progeny[6] != NULL && cj->progeny[3] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[6], cj->progeny[3], -1,
                                          0);
        break;

      case 6: /* (  1 , -1 ,  1 ) */
        if (ci->progeny[5] != NULL && cj->progeny[2] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[5], cj->progeny[2], -1,
                                          0);
        break;

      case 7: /* (  1 , -1 ,  0 ) */
        if (ci->progeny[4] != NULL && cj->progeny[2] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[4], cj->progeny[2], -1,
                                          0);
        if (ci->progeny[4] != NULL && cj->progeny[3] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[4], cj->progeny[3], -1,
                                          0);
        if (ci->progeny[5] != NULL && cj->progeny[2] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[5], cj->progeny[2], -1,
                                          0);
        if (ci->progeny[5] != NULL && cj->progeny[3] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[5], cj->progeny[3], -1,
                                          0);
        break;

      case 8: /* (  1 , -1 , -1 ) */
        if (ci->progeny[4] != NULL && cj->progeny[3] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[4], cj->progeny[3], -1,
                                          0);
        break;

      case 9: /* (  0 ,  1 ,  1 ) */
        if (ci->progeny[3] != NULL && cj->progeny[0] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[3], cj->progeny[0], -1,
                                          0);
        if (ci->progeny[3] != NULL && cj->progeny[4] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[3], cj->progeny[4], -1,
                                          0);
        if (ci->progeny[7] != NULL && cj->progeny[0] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[7], cj->progeny[0], -1,
                                          0);
        if (ci->progeny[7] != NULL && cj->progeny[4] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[7], cj->progeny[4], -1,
                                          0);
        break;

      case 10: /* (  0 ,  1 ,  0 ) */
        if (ci->progeny[2] != NULL && cj->progeny[0] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[2], cj->progeny[0], -1,
                                          0);
        if (ci->progeny[2] != NULL && cj->progeny[1] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[2], cj->progeny[1], -1,
                                          0);
        if (ci->progeny[2] != NULL && cj->progeny[4] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[2], cj->progeny[4], -1,
                                          0);
        if (ci->progeny[2] != NULL && cj->progeny[5] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[2], cj->progeny[5], -1,
                                          0);
        if (ci->progeny[3] != NULL && cj->progeny[0] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[3], cj->progeny[0], -1,
                                          0);
        if (ci->progeny[3] != NULL && cj->progeny[1] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[3], cj->progeny[1], -1,
                                          0);
        if (ci->progeny[3] != NULL && cj->progeny[4] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[3], cj->progeny[4], -1,
                                          0);
        if (ci->progeny[3] != NULL && cj->progeny[5] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[3], cj->progeny[5], -1,
                                          0);
        if (ci->progeny[6] != NULL && cj->progeny[0] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[6], cj->progeny[0], -1,
                                          0);
        if (ci->progeny[6] != NULL && cj->progeny[1] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[6], cj->progeny[1], -1,
                                          0);
        if (ci->progeny[6] != NULL && cj->progeny[4] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[6], cj->progeny[4], -1,
                                          0);
        if (ci->progeny[6] != NULL && cj->progeny[5] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[6], cj->progeny[5], -1,
                                          0);
        if (ci->progeny[7] != NULL && cj->progeny[0] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[7], cj->progeny[0], -1,
                                          0);
        if (ci->progeny[7] != NULL && cj->progeny[1] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[7], cj->progeny[1], -1,
                                          0);
        if (ci->progeny[7] != NULL && cj->progeny[4] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[7], cj->progeny[4], -1,
                                          0);
        if (ci->progeny[7] != NULL && cj->progeny[5] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[7], cj->progeny[5], -1,
                                          0);
        break;

      case 11: /* (  0 ,  1 , -1 ) */
        if (ci->progeny[2] != NULL && cj->progeny[1] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[2], cj->progeny[1], -1,
                                          0);
        if (ci->progeny[2] != NULL && cj->progeny[5] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[2], cj->progeny[5], -1,
                                          0);
        if (ci->progeny[6] != NULL && cj->progeny[1] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[6], cj->progeny[1], -1,
                                          0);
        if (ci->progeny[6] != NULL && cj->progeny[5] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[6], cj->progeny[5], -1,
                                          0);
        break;

      case 12: /* (  0 ,  0 ,  1 ) */
        if (ci->progeny[1] != NULL && cj->progeny[0] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[1], cj->progeny[0], -1,
                                          0);
        if (ci->progeny[1] != NULL && cj->progeny[2] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[1], cj->progeny[2], -1,
                                          0);
        if (ci->progeny[1] != NULL && cj->progeny[4] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[1], cj->progeny[4], -1,
                                          0);
        if (ci->progeny[1] != NULL && cj->progeny[6] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[1], cj->progeny[6], -1,
                                          0);
        if (ci->progeny[3] != NULL && cj->progeny[0] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[3], cj->progeny[0], -1,
                                          0);
        if (ci->progeny[3] != NULL && cj->progeny[2] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[3], cj->progeny[2], -1,
                                          0);
        if (ci->progeny[3] != NULL && cj->progeny[4] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[3], cj->progeny[4], -1,
                                          0);
        if (ci->progeny[3] != NULL && cj->progeny[6] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[3], cj->progeny[6], -1,
                                          0);
        if (ci->progeny[5] != NULL && cj->progeny[0] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[5], cj->progeny[0], -1,
                                          0);
        if (ci->progeny[5] != NULL && cj->progeny[2] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[5], cj->progeny[2], -1,
                                          0);
        if (ci->progeny[5] != NULL && cj->progeny[4] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[5], cj->progeny[4], -1,
                                          0);
        if (ci->progeny[5] != NULL && cj->progeny[6] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[5], cj->progeny[6], -1,
                                          0);
        if (ci->progeny[7] != NULL && cj->progeny[0] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[7], cj->progeny[0], -1,
                                          0);
        if (ci->progeny[7] != NULL && cj->progeny[2] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[7], cj->progeny[2], -1,
                                          0);
        if (ci->progeny[7] != NULL && cj->progeny[4] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[7], cj->progeny[4], -1,
                                          0);
        if (ci->progeny[7] != NULL && cj->progeny[6] != NULL)
          runner_dosub_pair_stars_density(r, ci->progeny[7], cj->progeny[6], -1,
                                          0);
        break;
    }

  }

  /* Otherwise, compute the pair directly. */
  else if (cell_is_active_stars(ci, e) || cell_is_active_stars(cj, e)) {

    /* Make sure both cells are drifted to the current timestep. */
    if (!cell_are_part_drifted(ci, e) || !cell_are_part_drifted(cj, e))
      error("Interacting undrifted cells.");

    /* Do any of the cells need to be sorted first? */
    if (!(ci->sorted & (1 << sid)) ||
        ci->dx_max_sort_old > ci->dmin * space_maxreldx)
      error("Interacting unsorted cell.");
    if (!(cj->sorted & (1 << sid)) ||
        cj->dx_max_sort_old > cj->dmin * space_maxreldx)
      error("Interacting unsorted cell.");

    /* Compute the interactions. */
    runner_dopair_branch_stars_density(r, ci, cj);
  }

  if (gettimer) TIMER_TOC(TIMER_DOSUB_PAIR);
}

/**
 * @brief Compute grouped sub-cell interactions for self tasks
 *
 * @param r The #runner.
 * @param ci The first #cell.
 * @param gettimer Do we have a timer ?
 */
void runner_dosub_self_stars_density(struct runner *r, struct cell *ci,
                                     int gettimer) {

  TIMER_TIC;

  /* Should we even bother? */
  if (ci->scount == 0 || !cell_is_active_stars(ci, r->e)) return;

  /* Recurse? */
  if (cell_can_recurse_in_self_stars_task(ci)) {

    /* Loop over all progeny. */
    for (int k = 0; k < 8; k++)
      if (ci->progeny[k] != NULL) {
        runner_dosub_self_stars_density(r, ci->progeny[k], 0);
        for (int j = k + 1; j < 8; j++)
          if (ci->progeny[j] != NULL)
            runner_dosub_pair_stars_density(r, ci->progeny[k], ci->progeny[j],
                                            -1, 0);
      }
  }

  /* Otherwise, compute self-interaction. */
  else {

    runner_doself_branch_stars_density(r, ci);
  }

  if (gettimer) TIMER_TOC(timer_dosub_self_stars_density);
}