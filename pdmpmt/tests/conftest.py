"""pytest fixtures for Monte Carlo pi tests.

.. codeauthor:: Derek Huang <djh458@stern.nyu.edu>
"""

import pytest


@pytest.fixture(scope="session")
def n_serial_samples() -> int:
    """Number of samples to use to estimate pi in a single process.

    Note that using this for n_samples results in a very crude guess.
    """
    return 1000000


@pytest.fixture(scope="session")
def n_batches() -> int:
    """Number of parallel batches to use for parallel pi estimation."""
    return 10


@pytest.fixture(scope="session")
def default_seed() -> int:
    """Default NumPy PRNG seed to use."""
    return 7


@pytest.fixture(scope="session")
def big_tol() -> float:
    """Largest tolerance used for checking pi estimates.

    Requried since n_serial_samples is only accurate to 2 significant figures.
    """
    return 1e-2
