/*******************************************************************************
 * This file is part of SWIFT.
 * Coypright (c) 2016 Matthieu Schaller (matthieu.schaller@durham.ac.uk)
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

/**
 * @brief Reads the different particles to the HDF5 file
 *
 * @param h_grp The HDF5 group in which to read the arrays.
 * @param N The number of particles on that MPI rank.
 * @param N_total The total number of particles (only used in MPI mode)
 * @param offset The offset of the particles for this MPI rank (only used in MPI
 * mode)
 * @param parts The particle array
 * @param internal_units The #UnitSystem used internally
 * @param ic_units The #UnitSystem used in the snapshots
 *
 */
__attribute__((always_inline)) INLINE static void hydro_read_particles(
    hid_t h_grp, int N, long long N_total, long long offset, struct part* parts,
    const struct UnitSystem* internal_units, struct UnitSystem* ic_units) {

  const int num_fields = 8;
  struct io_props list[num_fields];

  /* List what we want to read */
  list[0] = io_make_input_field("Coordinates", DOUBLE, 3, COMPULSORY,
                                UNIT_CONV_LENGTH, parts, x);
  list[1] = io_make_input_field("Velocities", FLOAT, 3, COMPULSORY,
                                UNIT_CONV_SPEED, parts, v);
  list[2] = io_make_input_field("Masses", FLOAT, 1, COMPULSORY, UNIT_CONV_MASS,
                                parts, mass);
  list[3] = io_make_input_field("SmoothingLength", FLOAT, 1, COMPULSORY,
                                UNIT_CONV_LENGTH, parts, h);
  list[4] = io_make_input_field("InternalEnergy", FLOAT, 1, COMPULSORY,
                                UNIT_CONV_ENERGY, parts, entropy);
  list[5] = io_make_input_field("ParticleIDs", ULONGLONG, 1, COMPULSORY,
                                UNIT_CONV_NO_UNITS, parts, id);
  list[6] = io_make_input_field("Accelerations", FLOAT, 3, OPTIONAL,
                                UNIT_CONV_ACCELERATION, parts, a_hydro);
  list[7] = io_make_input_field("Density", FLOAT, 1, OPTIONAL,
                                UNIT_CONV_DENSITY, parts, rho);

  /* Read arrays */
  /* readArray(h_grp, "Coordinates", DOUBLE, N, 3, parts, N_total, offset, x, */
  /*           COMPULSORY, internal_units, ic_units, UNIT_CONV_LENGTH); */
  /* readArray(h_grp, "Velocities", FLOAT, N, 3, parts, N_total, offset, v, */
  /*           COMPULSORY, internal_units, ic_units, UNIT_CONV_SPEED); */
  /* readArray(h_grp, "Masses", FLOAT, N, 1, parts, N_total, offset, mass, */
  /*           COMPULSORY, internal_units, ic_units, UNIT_CONV_MASS); */
  /* readArray(h_grp, "SmoothingLength", FLOAT, N, 1, parts, N_total, offset, h,
   */
  /*           COMPULSORY, internal_units, ic_units, UNIT_CONV_LENGTH); */
  /* readArray(h_grp, "InternalEnergy", FLOAT, N, 1, parts, N_total, offset, */
  /*           entropy, COMPULSORY, internal_units, ic_units, UNIT_CONV_ENERGY);
   */
  /* readArray(h_grp, "ParticleIDs", ULONGLONG, N, 1, parts, N_total, offset,
   * id, */
  /*           COMPULSORY, internal_units, ic_units, UNIT_CONV_NO_UNITS); */
  /* readArray(h_grp, "Acceleration", FLOAT, N, 3, parts, N_total, offset,
   * a_hydro, */
  /*           OPTIONAL, internal_units, ic_units, UNIT_CONV_ACCELERATION); */
  /* readArray(h_grp, "Density", FLOAT, N, 1, parts, N_total, offset, rho, */
  /*           OPTIONAL, internal_units, ic_units, UNIT_CONV_DENSITY); */

  /* And read everything */
  for (int i = 0; i < num_fields; ++i)
    readArray(h_grp, list[i], N, N_total, offset, internal_units, ic_units);
}

/**
 * @brief Writes the different particles to the HDF5 file
 *
 * @param h_grp The HDF5 group in which to write the arrays.
 * @param fileName The name of the file (unsued in MPI mode).
 * @param partTypeGroupName The name of the group containing the particles in
 * the HDF5 file.
 * @param xmfFile The XMF file to write to (unused in MPI mode).
 * @param N The number of particles on that MPI rank.
 * @param N_total The total number of particles (only used in MPI mode)
 * @param mpi_rank The MPI rank of this node (only used in MPI mode)
 * @param offset The offset of the particles for this MPI rank (only used in MPI
 * mode)
 * @param parts The particle array
 * @param internal_units The #UnitSystem used internally
 * @param snapshot_units The #UnitSystem used in the snapshots
 *
 */
__attribute__((always_inline)) INLINE static void hydro_write_particles(
    hid_t h_grp, char* fileName, char* partTypeGroupName, FILE* xmfFile, int N,
    long long N_total, int mpi_rank, long long offset, struct part* parts,
    const struct UnitSystem* internal_units,
    const struct UnitSystem* snapshot_units) {

  /* Write arrays */
  /* writeArray(h_grp, fileName, xmfFile, partTypeGroupName, "Coordinates",
   * DOUBLE, */
  /*            N, 3, parts, N_total, mpi_rank, offset, x, internal_units, */
  /*            snapshot_units, UNIT_CONV_LENGTH); */
  /* writeArray(h_grp, fileName, xmfFile, partTypeGroupName, "Velocities",
   * FLOAT, */
  /*            N, 3, parts, N_total, mpi_rank, offset, v, internal_units, */
  /*            snapshot_units, UNIT_CONV_SPEED); */
  /* writeArray(h_grp, fileName, xmfFile, partTypeGroupName, "Masses", FLOAT, N,
   * 1, */
  /*            parts, N_total, mpi_rank, offset, mass, internal_units, */
  /*            snapshot_units, UNIT_CONV_MASS); */
  /* writeArray(h_grp, fileName, xmfFile, partTypeGroupName, "SmoothingLength",
   */
  /*            FLOAT, N, 1, parts, N_total, mpi_rank, offset, h,
   * internal_units, */
  /*            snapshot_units, UNIT_CONV_LENGTH); */
  /* writeArray(h_grp, fileName, xmfFile, partTypeGroupName, "Entropy", FLOAT,
   * N, */
  /*            1, parts, N_total, mpi_rank, offset, entropy, internal_units, */
  /*            snapshot_units, UNIT_CONV_ENTROPY_PER_UNIT_MASS); */
  /* writeArray(h_grp, fileName, xmfFile, partTypeGroupName, "ParticleIDs", */
  /*            ULONGLONG, N, 1, parts, N_total, mpi_rank, offset, id, */
  /*            internal_units, snapshot_units, UNIT_CONV_NO_UNITS); */
  /* writeArray(h_grp, fileName, xmfFile, partTypeGroupName, "Acceleration",
   * FLOAT, */
  /*            N, 3, parts, N_total, mpi_rank, offset, a_hydro, internal_units,
   */
  /*            snapshot_units, UNIT_CONV_ACCELERATION); */
  /* writeArray(h_grp, fileName, xmfFile, partTypeGroupName, "Density", FLOAT,
   * N, */
  /*            1, parts, N_total, mpi_rank, offset, rho, internal_units, */
  /*            snapshot_units, UNIT_CONV_DENSITY); */
}

/**
 * @brief Writes the current model of SPH to the file
 * @param h_grpsph The HDF5 group in which to write
 */
void writeSPHflavour(hid_t h_grpsph) {

  /* Kernel function description */
  writeAttribute_s(h_grpsph, "Kernel", kernel_name);

  /* Viscosity and thermal conduction */
  writeAttribute_s(h_grpsph, "Thermal Conductivity Model",
                   "(No treatment) Legacy Gadget-2 as in Springel (2005)");
  writeAttribute_s(h_grpsph, "Viscosity Model",
                   "Legacy Gadget-2 as in Springel (2005)");
  writeAttribute_f(h_grpsph, "Viscosity alpha", const_viscosity_alpha);
  writeAttribute_f(h_grpsph, "Viscosity beta", 3.f);
}

/**
 * @brief Are we writing entropy in the internal energy field ?
 *
 * @return 1 if entropy is in 'internal energy', 0 otherwise.
 */
int writeEntropyFlag() { return 1; }
