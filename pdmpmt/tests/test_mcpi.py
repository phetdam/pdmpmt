"""Tests for the mcpi module.

.. codeauthor:: Derek Huang <djh458@stern.nyu.edu>
"""

import math

import numpy as np

# pyright: reportMissingImports=false
from pdmpmt.mcpi import (
    generate_seeds, mcpi_dask, mcpi_gather, mcpi_serial, unit_circle_samples
)


def test_mcpi_serial(n_serial_samples, default_seed, big_tol):
    """Test that the serial Monte Carlo pi implementation works.

    Parameters
    ----------
    n_serial_samples : int
        pytest fixture, see conftest.py
    default_seed : int
        pytest fixture, see conftest.py
    big_tol : float
        pytest fixture, see conftest.py
    """
    mcpi = mcpi_serial(n_samples=n_serial_samples, seed=default_seed)
    # the estimate is pretty rough with low samples so ballpark 3.14 is fine
    np.testing.assert_allclose(mcpi, math.pi, atol=big_tol)


def test_mcpi_gather(n_serial_samples, n_batches, default_seed, big_tol):
    """Test that the parallel Monte Carlo pi implementation reduce works.

    We do this in a serial context, with around same complexity as serial. See
    test_mcpi_serial for documentation on unlisted parameters.

    Parameters
    ----------
    n_batches : int
        pytest fixture, see conftest.py
    """
    seeds = generate_seeds(n_batches, initial_seed=default_seed)
    # division has no remainder here, but we also explicitly check
    assert n_serial_samples % n_batches == 0, "uneven division of samples"
    n_batch_samples = n_serial_samples // n_batches
    circle_counts = [
        unit_circle_samples(n_batch_samples, seed=seed)
        for seed in seeds
    ]
    mcpi = mcpi_gather(circle_counts, [n_batch_samples] * n_batches)
    # same tolerance as test_mcpi_serial
    np.testing.assert_allclose(mcpi, math.pi, atol=big_tol)


def test_mcpi_dask(n_serial_samples, default_seed, big_tol):
    """Test that parallel pi estimation using local Dask cluster works.

    See test_mcpi_serial for parameter documentation.
    """
    # we don't need that many batches
    n_batches = 2
    # run using LocalCluster with 2 workers, 2 jobs
    mcpi = mcpi_dask(
        n_samples=n_serial_samples,
        seed=default_seed,
        n_jobs=n_batches,
        n_workers=n_batches
    )
    np.testing.assert_allclose(mcpi, math.pi, atol=big_tol)
