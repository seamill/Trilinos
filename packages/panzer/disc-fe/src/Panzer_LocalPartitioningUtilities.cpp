// @HEADER
// ***********************************************************************
//
//           Panzer: A partial differential equation assembly
//       engine for strongly coupled complex multiphysics systems
//                 Copyright (2011) Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact Roger P. Pawlowski (rppawlo@sandia.gov) and
// Eric C. Cyr (eccyr@sandia.gov)
// ***********************************************************************
// @HEADER

#include "Panzer_LocalPartitioningUtilities.hpp"

#include "Teuchos_RCP.hpp"
#include "Teuchos_Comm.hpp"
#include "Teuchos_Assert.hpp"

#include "Panzer_WorksetUtilities.hpp"
#include "Panzer_WorksetDescriptor.hpp"

#include "Phalanx_KokkosDeviceTypes.hpp"

#include <unordered_set>
#include <unordered_map>

namespace panzer
{

namespace partitioning_utilities
{

void
setupSubLocalMeshInfoWithConnectivity(const panzer::LocalMeshInfoBase & parent_info,
                                      const std::vector<size_t> & owned_parent_cells,
                                      panzer::LocalMeshInfoBase & sub_info)
{
  using GO = panzer::GlobalOrdinal;
  using LO = panzer::LocalOrdinal;

  PANZER_FUNC_TIME_MONITOR_DIFF("panzer::partitioning_utilities::setupSubLocalMeshInfo",setupSLMI);
  // The goal of this function is to fill a LocalMeshInfoBase (sub_info) with
  // a subset of cells from a given parent LocalMeshInfoBase (parent_info)

  // Note: owned_parent_cells are the owned cells for sub_info in the parent_info's indexing scheme
  // We need to generate sub_info's ghosts and figure out the virtual cells

  // Note: We will only handle a single ghost layer

  // Note: We assume owned_parent_cells are owned cells of the parent
  // i.e. owned_parent_indexes cannot refer to ghost or virtual cells in parent_info

  // Note: This function works with inter-face connectivity. NOT node connectivity.

  const size_t num_owned_cells = owned_parent_cells.size();
  TEUCHOS_TEST_FOR_EXCEPT_MSG(num_owned_cells == 0, "panzer::partitioning_utilities::setupSubLocalMeshInfoWithConnectivity : Input parent subcells must exist (owned_parent_cells)");

  const size_t num_parent_owned_cells = parent_info.num_owned_cells;
  TEUCHOS_TEST_FOR_EXCEPT_MSG(num_parent_owned_cells <= 0, "panzer::partitioning_utilities::setupSubLocalMeshInfoWithConnectivity : Input parent info must contain owned cells");

  const size_t num_parent_ghstd_cells = parent_info.num_ghost_cells;

  const size_t num_parent_total_cells = parent_info.num_owned_cells + parent_info.num_ghost_cells + parent_info.num_virtual_cells;

  // Just as a precaution, make sure the parent_info is setup properly
  TEUCHOS_ASSERT(static_cast<int>(parent_info.cell_to_faces.extent(0)) == num_parent_total_cells);
  const int num_faces_per_cell = parent_info.cell_to_faces.extent(1);

  // The first thing to do is construct a vector containing the parent cell indexes of all
  // owned, ghstd, and virtual cells
  std::vector<size_t> ghstd_parent_cells;
  std::vector<size_t> virtual_parent_cells;
  {
    PANZER_FUNC_TIME_MONITOR_DIFF("Construct parent cell vector",ParentCell);
    // We grab all of the owned cells and put their global indexes into sub_info
    // We also put all of the owned cell indexes in the parent's indexing scheme into a set to use for lookups
    std::unordered_set<size_t> owned_parent_cells_set(owned_parent_cells.begin(), owned_parent_cells.end());

    // We need to create a list of ghstd and virtual cells
    // We do this by running through sub_cell_indexes
    // and looking at the neighbors to find neighbors that are not owned

    // Virtual cells are defined as cells with indexes outside of the range of owned_cells and ghstd_cells
    const size_t num_parent_real_cells = num_parent_owned_cells + num_parent_ghstd_cells;

    std::unordered_set<size_t> ghstd_parent_cells_set;
    std::unordered_set<size_t> virtual_parent_cells_set;
    for(int i=0;i<num_owned_cells;++i){
      const size_t parent_cell_index = owned_parent_cells[i];
      for(int local_face_index=0;local_face_index<num_faces_per_cell;++local_face_index){
        const LO parent_face = parent_info.cell_to_faces(parent_cell_index, local_face_index);

        // Sidesets can have owned cells that border the edge of the domain (i.e. parent_face == -1)
        // If we are at the edge of the domain, we can ignore this face.
        if(parent_face < 0)
          continue;

        // Find the side index for neighbor cell with respect to the face
        const LO neighbor_parent_side = (parent_info.face_to_cells(parent_face,0) == parent_cell_index) ? 1 : 0;

        // Get the neighbor cell index in the parent's indexing scheme
        const LO neighbor_parent_cell = parent_info.face_to_cells(parent_face, neighbor_parent_side);

        // If the face exists, then the neighbor should exist
        TEUCHOS_ASSERT(neighbor_parent_cell >= 0);

        // We can easily check if this is a virtual cell
        if(neighbor_parent_cell >= num_parent_real_cells){
          virtual_parent_cells_set.insert(neighbor_parent_cell);
        } else if(neighbor_parent_cell >= num_parent_owned_cells){
          // This is a quick check for a ghost cell
          // This branch only exists to cut down on the number of times the next branch (much slower) is called
          ghstd_parent_cells_set.insert(neighbor_parent_cell);
        } else {
          // There is still potential for this to be a ghost cell with respect to 'our' cells
          // The only way to check this is with a super slow lookup call
          if(owned_parent_cells_set.find(neighbor_parent_cell) == owned_parent_cells_set.end()){
            // The neighbor cell is not owned by 'us', therefore it is a ghost
            ghstd_parent_cells_set.insert(neighbor_parent_cell);
          }
        }
      }
    }

    // We now have a list of the owned, ghstd, and virtual cells in the parent's indexing scheme.
    // We will take the 'unordered_set's ordering for the the sub-indexing scheme

    ghstd_parent_cells.assign(ghstd_parent_cells_set.begin(), ghstd_parent_cells_set.end());
    virtual_parent_cells.assign(virtual_parent_cells_set.begin(), virtual_parent_cells_set.end());

  }

  const size_t num_ghost_cells = ghstd_parent_cells.size();
  const size_t num_virtual_cells = virtual_parent_cells.size();
  const size_t num_real_cells = num_owned_cells + num_ghost_cells;
  const size_t num_total_cells = num_real_cells + num_virtual_cells;

  std::vector<std::pair<size_t, size_t> > all_parent_cells(num_total_cells);
  for (std::size_t i=0; i< owned_parent_cells.size(); ++i)
    all_parent_cells[i] = std::make_pair(owned_parent_cells[i], i);

  for (std::size_t i=0; i< ghstd_parent_cells.size(); ++i) {
    const size_t insert = num_owned_cells+i;
    all_parent_cells[insert] = std::make_pair(ghstd_parent_cells[i], insert);
  }

  for (std::size_t i=0; i< virtual_parent_cells.size(); ++i) {
    const size_t insert = num_real_cells+i;
    all_parent_cells[insert] = std::make_pair(virtual_parent_cells[i], insert);
  }

  sub_info.num_owned_cells = num_owned_cells;
  sub_info.num_ghost_cells = num_ghost_cells;
  sub_info.num_virtual_cells = num_virtual_cells;

  // We now have the indexing order for our sub_info

  // Just as a precaution, make sure the parent_info is setup properly
  TEUCHOS_ASSERT(static_cast<int>(parent_info.cell_vertices.extent(0)) == num_parent_total_cells);
  TEUCHOS_ASSERT(static_cast<int>(parent_info.local_cells.extent(0)) == num_parent_total_cells);
  TEUCHOS_ASSERT(static_cast<int>(parent_info.global_cells.extent(0)) == num_parent_total_cells);

  const int num_vertices_per_cell = parent_info.cell_vertices.extent(1);
  const int num_dims = parent_info.cell_vertices.extent(2);

  // Fill owned, ghstd, and virtual cells: global indexes, local indexes and vertices
  sub_info.global_cells = Kokkos::View<GO*>("global_cells", num_total_cells);
  sub_info.local_cells = Kokkos::View<LO*>("local_cells", num_total_cells);
  sub_info.cell_vertices = Kokkos::View<double***, PHX::Device>("cell_vertices", num_total_cells, num_vertices_per_cell, num_dims);
  for(int cell=0;cell<num_total_cells;++cell){
    const LO parent_cell = all_parent_cells[cell].first;
    sub_info.global_cells(cell) = parent_info.global_cells(parent_cell);
    sub_info.local_cells(cell) = parent_info.local_cells(parent_cell);
    for(int vertex=0;vertex<num_vertices_per_cell;++vertex){
      for(int dim=0;dim<num_dims;++dim){
        sub_info.cell_vertices(cell,vertex,dim) = parent_info.cell_vertices(parent_cell,vertex,dim);
      }
    }
  }

  // Now for the difficult part

  // We need to create a new face indexing scheme from the old face indexing scheme

  // Create an auxiliary list with all cells - note this preserves indexing

  struct face_t{
    face_t(LO c0, LO c1, LO sc0, LO sc1)
    {
      cell_0=c0;
      cell_1=c1;
      subcell_index_0=sc0;
      subcell_index_1=sc1;
    }
    LO cell_0;
    LO cell_1;
    LO subcell_index_0;
    LO subcell_index_1;
  };


  // First create the faces
  std::vector<face_t> faces;
  {
    PANZER_FUNC_TIME_MONITOR_DIFF("Create faces",CreateFaces);
    // faces_set: cell_0, subcell_index_0, cell_1, subcell_index_1
    std::unordered_map<LO,std::unordered_map<LO, std::pair<LO,LO> > > faces_set;

    std::sort(all_parent_cells.begin(), all_parent_cells.end());

    for(int owned_cell=0;owned_cell<num_owned_cells;++owned_cell){
      const LO owned_parent_cell = owned_parent_cells[owned_cell];
      for(int local_face=0;local_face<num_faces_per_cell;++local_face){
        const LO parent_face = parent_info.cell_to_faces(owned_parent_cell,local_face);

        // Skip faces at the edge of the domain
        if(parent_face<0)
          continue;

        // Get the cell on the other side of the face
        const LO neighbor_side = (parent_info.face_to_cells(parent_face,0) == owned_parent_cell) ? 1 : 0;

        const LO neighbor_parent_cell = parent_info.face_to_cells(parent_face, neighbor_side);
        const LO neighbor_subcell_index = parent_info.face_to_lidx(parent_face, neighbor_side);

        // Convert parent cell index into sub cell index
        std::pair<size_t, size_t> search_point(neighbor_parent_cell, 0);
        auto itr = std::lower_bound(all_parent_cells.begin(), all_parent_cells.end(), search_point);

        TEUCHOS_TEST_FOR_EXCEPT_MSG(itr == all_parent_cells.end(), "panzer_stk::setupSubLocalMeshInfoWithConnectivity : Neighbor cell was not found in owned, ghosted, or virtual cells");

        const LO neighbor_cell = itr->second;

        LO cell_0, cell_1, subcell_index_0, subcell_index_1;
        if(owned_cell < neighbor_cell){
          cell_0 = owned_cell;
          subcell_index_0 = local_face;
          cell_1 = neighbor_cell;
          subcell_index_1 = neighbor_subcell_index;
        } else {
          cell_1 = owned_cell;
          subcell_index_1 = local_face;
          cell_0 = neighbor_cell;
          subcell_index_0 = neighbor_subcell_index;
        }

        // Add this interface to the set of faces - smaller cell index is 'left' (or '0') side of face
        faces_set[cell_0][subcell_index_0] = std::pair<LO,LO>(cell_1, subcell_index_1);
      }
    }

    for(const auto & cell_pair : faces_set){
      const LO cell_0 = cell_pair.first;
      for(const auto & subcell_pair : cell_pair.second){
        const LO subcell_index_0 = subcell_pair.first;
        const LO cell_1 = subcell_pair.second.first;
        const LO subcell_index_1 = subcell_pair.second.second;
        faces.push_back(face_t(cell_0,cell_1,subcell_index_0,subcell_index_1));
      }
    }
  }

  const int num_faces = faces.size();

  sub_info.has_connectivity = parent_info.has_connectivity;
  sub_info.face_to_cells = Kokkos::View<LO*[2]>("face_to_cells", num_faces);
  sub_info.face_to_lidx = Kokkos::View<LO*[2]>("face_to_lidx", num_faces);
  sub_info.cell_to_faces = Kokkos::View<LO**>("cell_to_faces", num_total_cells, num_faces_per_cell);

  // Default the system with invalid cell index
  Kokkos::deep_copy(sub_info.cell_to_faces, -1);

  for(int face_index=0;face_index<num_faces;++face_index){
    const face_t & face = faces[face_index];

    sub_info.face_to_cells(face_index,0) = face.cell_0;
    sub_info.face_to_cells(face_index,1) = face.cell_1;

    sub_info.cell_to_faces(face.cell_0,face.subcell_index_0) = face_index;
    sub_info.cell_to_faces(face.cell_1,face.subcell_index_1) = face_index;

    sub_info.face_to_lidx(face_index,0) = face.subcell_index_0;
    sub_info.face_to_lidx(face_index,1) = face.subcell_index_1;

  }

  sub_info.has_connectivity = true;

  // This interface is half-implemented so we need to only use it if it exists
  if(not parent_info.cell_sets.is_null()){
    std::vector<int> sub_cells(num_total_cells);
    if(parent_info.cell_sets->getNumElements() > 0)
      for(size_t i=0; i<num_total_cells; ++i)
        sub_cells[all_parent_cells[i].second] = all_parent_cells[i].first;
    sub_info.cell_sets = parent_info.cell_sets->buildSubsets(sub_cells);
  }

  // Complete.

}

void
setupSubLocalMeshInfo(const panzer::LocalMeshInfoBase & parent_info,
                      const std::vector<size_t> & owned_parent_cells,
                      panzer::LocalMeshInfoBase & sub_info)
{

  using GO = panzer::GlobalOrdinal;
  using LO = panzer::LocalOrdinal;

  PANZER_FUNC_TIME_MONITOR_DIFF("panzer::partitioning_utilities::setupSubLocalMeshInfo",setupSLMI);

  // This call does not add connectivity or ghost cells into the partition.
  // It builds the bare necessities required for a local mesh partition.

  // Need to make sure we have at least one cell
  TEUCHOS_ASSERT(parent_info.num_owned_cells > 0);
  TEUCHOS_ASSERT(owned_parent_cells.size() > 0);
  TEUCHOS_ASSERT(owned_parent_cells.size() <= parent_info.num_owned_cells);

  // Setup basic size info
  sub_info.num_owned_cells = owned_parent_cells.size();
  sub_info.num_ghost_cells = 0;
  sub_info.num_virtual_cells = 0;

  const unsigned long num_total_cells = owned_parent_cells.size();

  // Build indexing arrays
  sub_info.local_cells = Kokkos::View<LO*>("local_cells",num_total_cells);
  sub_info.global_cells = Kokkos::View<GO*>("global_cells",num_total_cells);
  for(size_t i=0; i<num_total_cells; ++i){
    sub_info.local_cells(i) = parent_info.local_cells(owned_parent_cells[i]);
    sub_info.global_cells(i) = parent_info.global_cells(owned_parent_cells[i]);
  }

  // Add the cell vertices
  const int num_vertices_per_cell = parent_info.cell_vertices.extent(1);
  const int num_dims = parent_info.cell_vertices.extent(2);
  sub_info.cell_vertices = Kokkos::View<double***>("cell_vertices",num_total_cells, num_vertices_per_cell, num_dims);
  for(int cell=0;cell<num_total_cells;++cell){
    const LO & parent_cell = owned_parent_cells[cell];
    for(int vertex=0;vertex<num_vertices_per_cell;++vertex)
      for(int dim=0;dim<num_dims;++dim)
        sub_info.cell_vertices(cell, vertex, dim) = parent_info.cell_vertices(parent_cell, vertex, dim);
  }

  // This should probably just be false
  sub_info.has_connectivity = parent_info.has_connectivity;

  // This interface is half-implemented so we need to only use it if it exists
  if(not parent_info.cell_sets.is_null()){
    std::vector<int> sub_cells(num_total_cells);
    if(parent_info.cell_sets->getNumElements() > 0)
      for(size_t i=0; i<owned_parent_cells.size(); ++i)
        sub_cells[i] = owned_parent_cells[i];
    sub_info.cell_sets = parent_info.cell_sets->buildSubsets(sub_cells);
  }

  // Complete

}

}

namespace
{

void
splitMeshInfo(const panzer::LocalMeshInfoBase & mesh_info,
              const long splitting_size,
              const bool include_connectivity,
              std::vector<panzer::LocalMeshPartition > & partitions)
{

  // Make sure the splitting size makes sense
  TEUCHOS_ASSERT((splitting_size > 0) or (splitting_size == WorksetSizeType::ALL_ELEMENTS));

  // Default partition size
  const long base_partition_size = (splitting_size > 0) ? splitting_size : mesh_info.num_owned_cells;

  // Cells to partition
  std::vector<size_t> partition_cells;
  partition_cells.resize(base_partition_size);

  // Create the partitions
  size_t cell_count = 0;
  while(cell_count < mesh_info.num_owned_cells){

    size_t partition_size = base_partition_size;
    if(cell_count + partition_size > mesh_info.num_owned_cells)
      partition_size = mesh_info.num_owned_cells - cell_count;

    // Error check for a null partition - this should never happen by design
    TEUCHOS_ASSERT(partition_size != 0);

    // In the final partition, we need to reduce the size of partition_cells
    if(partition_size != base_partition_size)
      partition_cells.resize(partition_size);

    // Set the partition indexes - not really a partition, just a chunk of cells
    for(size_t i=0; i<partition_size; ++i)
      partition_cells[i] = cell_count+i;

    // Create an empty partition
    partitions.push_back(panzer::LocalMeshPartition());

    // Fill the empty partition
    if(include_connectivity)
      partitioning_utilities::setupSubLocalMeshInfoWithConnectivity(mesh_info,partition_cells,partitions.back());
    else
      partitioning_utilities::setupSubLocalMeshInfo(mesh_info,partition_cells,partitions.back());

    // Update the cell count
    cell_count += partition_size;
  }

}

void
generateVolumePartitions(const panzer::LocalMeshInfo & mesh_info,
                         const std::string & element_block_name,
                         const long owned_size,
                         const bool include_ghosts,
                         std::vector<panzer::LocalMeshPartition> & partitions)
{

  // Make sure the mesh has this element block
  if(mesh_info.element_blocks.find(element_block_name) == mesh_info.element_blocks.end())
    return;

  // Grab the element block we're interested in
  const panzer::LocalMeshBlockInfo & block_info = mesh_info.element_blocks.at(element_block_name);

  // Generate the partitions
  splitMeshInfo(block_info, owned_size, include_ghosts, partitions);

  // Additional metadata
  for(auto & partition : partitions){
    partition.element_block_name = element_block_name;
    partition.cell_topology = block_info.cell_topology;
  }

}

void
generateSidesetPartitions(const panzer::LocalMeshInfo & mesh_info,
                         const std::string & element_block_name,
                         const std::string & sideset_name,
                         const long owned_size,
                         const bool include_ghosts,
                         std::vector<panzer::LocalMeshPartition> & partitions)
{

  // Find the element block in the sideset options
  const auto element_block_itr = mesh_info.sidesets.find(element_block_name);

  // Make sure the mesh has this element block
  if(element_block_itr == mesh_info.sidesets.end())
    return;

  // The map of sidesets indexed by name
  const auto & sideset_map = element_block_itr->second;

  // Find the sideset
  const auto sideset_itr = sideset_map.find(sideset_name);

  // No partitions allowed if it doesn't exist
  if(sideset_itr == sideset_map.end())
    return;

  // Grab the sideset we're interested in
  const panzer::LocalMeshSidesetInfo & sideset_info = sideset_itr->second;

  // Generate the partitions
  splitMeshInfo(sideset_info, owned_size, include_ghosts, partitions);

  // Additional metadata
  for(auto & partition : partitions){
    partition.element_block_name = element_block_name;
    partition.sideset_name = sideset_name;
    partition.cell_topology = sideset_info.cell_topology;
  }

}

}

void
generateLocalMeshPartitions(const panzer::LocalMeshInfo & mesh_info,
                            const panzer::WorksetDescriptor & description,
                            std::vector<panzer::LocalMeshPartition> & partitions)
{
  // We have to make sure that the partitioning is possible
  TEUCHOS_ASSERT(description.getWorksetSize() != 0);

  const std::string & element_block_name = description.getElementBlock();

  // There are four options for worksets:

  // 1) Volume partitions for an element block
  // 2) Volume partitions for an element block + sideset
  // 3) Surface partitions for an element block + sideset
  // 4) Volume partitions for an element block + element_block + sideset (boundary between two element blocks)

  // Note that 3 and 4 can be represented by 1 and 2

  // Checking if there is a sideset breaks option 1 off from the others
  if(description.useSideset()){

    if(description.connectsElementBlocks()){

      // Option 4 - currently not supported by partitions
      TEUCHOS_ASSERT(false);

    } else {

      if(description.sideAssembly()){

        // Option 3 - currently not supported by partitions
        TEUCHOS_ASSERT(false);

      } else {

        // Option 2
        generateSidesetPartitions(mesh_info,
                                  description.getElementBlock(),
                                  description.getSideset(),
                                  description.getWorksetSize(),
                                  description.includeGhostCells(),
                                  partitions);

      }

    }

  } else {

    // Option 1
    generateVolumePartitions(mesh_info,
                             description.getElementBlock(),
                             description.getWorksetSize(),
                             description.includeGhostCells(),
                             partitions);

  }

}

}

