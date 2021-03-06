#include <deal.II/base/mpi.h>
#include <deal.II/distributed/tria.h>
#include <deal.II/fe/fe_q.h>
#include <deal.II/grid/grid_generator.h>
#include <deal.II/grid/grid_out.h>

#include <deal.II/distributed/tria_util.h>

const MPI_Comm comm = MPI_COMM_WORLD;

using namespace dealii;



template<int dim>
void
test(int n_refinements, const int n_subdivisions, MPI_Comm comm)
{
  // create pdt
  parallel::distributed::Triangulation<dim> tria_pdt(
    comm,
    dealii::Triangulation<dim>::none,
    parallel::distributed::Triangulation<dim>::construct_multigrid_hierarchy);
  GridGenerator::subdivided_hyper_cube(tria_pdt, n_subdivisions);
  tria_pdt.refine_global(n_refinements);

  // create instance of pft
  parallel::fullydistributed::Triangulation<dim> tria_pft(
    comm, parallel::fullydistributed::Triangulation<dim>::construct_multigrid_hierarchy);

  // extract relevant information form pdt
  auto construction_data =
    parallel::fullydistributed::Utilities::copy_from_triangulation(tria_pdt, tria_pft);

  // actually create triangulation
  tria_pft.reinit(construction_data);

  // test triangulation
  FE_Q<dim>       fe(2);
  DoFHandler<dim> dof_handler(tria_pft);
  dof_handler.distribute_dofs(fe);
  dof_handler.distribute_mg_dofs();

  // output meshes as VTU
  GridOut grid_out;
  grid_out.write_mesh_per_processor_as_vtu(tria_pdt, "trid_pdt", true, true);
  grid_out.write_mesh_per_processor_as_vtu(tria_pft, "trid_pft", true, true);
}



int
main(int argc, char ** argv)
{
  Utilities::MPI::MPI_InitFinalize mpi(argc, argv, 1);

  AssertThrow(argc > 3, ExcMessage("You have not provided two command-line arguments."));

  const int dim            = atoi(argv[1]);
  const int n_refinements  = atoi(argv[2]);
  const int n_subdivisions = atoi(argv[3]);

  ConditionalOStream pcout(std::cout, dealii::Utilities::MPI::this_mpi_process(comm) == 0);

  try
  {
    // clang-format off
    pcout << "Run step-8: "
          << " p=" << std::setw(2) << dealii::Utilities::MPI::n_mpi_processes(comm)
          << " d=" << std::setw(2) << dim 
          << " r=" << std::setw(2) << n_refinements
          << " s=" << std::setw(2) << n_subdivisions
          << ":";
    // clang-format on

    if(dim == 2)
      test<2>(n_refinements, n_subdivisions, comm);
    else if(dim == 3)
      test<3>(n_refinements, n_subdivisions, comm);
    else
      AssertThrow(false, ExcMessage("Only working for dimensions 2 and 3!"));
    pcout << " success...." << std::endl;
  }
  catch(...)
  {
    pcout << " failed...." << std::endl;
  }
}