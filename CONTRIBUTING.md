Contributing to OpenWeave
========================

Want to contribute? Great! First, read this page (including the small
print at the end).

## Bugs

If you find a bug in the source code, you can help us by [submitting a GitHub Issue](https://github.com/openweave/openweave-core/issues/new).  The best bug reports provide a detailed description of the issue and step-by-step instructions for predictably reproducing the issue.  Even better, you can [submit a Pull Request](#submitting-a-pull-request) with a fix.

## New Features

You can request a new feature by [submitting a GitHub Issue](https://github.com/openweave/openweave-core/issues/new).

If you would like to implement a new feature, please consider the scope of the new feature:

* *Large feature*: first [submit a GitHub
  Issue](https://github.com/openweave/openweave-core/issues/new) and communicate
  your proposal so that the community can review and provide feedback.  Getting
  early feedback will help ensure your implementation work is accepted by the
  community.  This will also allow us to better coordinate our efforts and
  minimize duplicated effort.

* *Small feature*: can be implemented and directly [submitted as a Pull
  Request](#submitting-a-pull-request).

## Contributing Code

The OpenWeave-Core follows the "Fork-and-Pull" model for accepting contributions.

### Initial Setup

Setup your GitHub fork and continuous-integration services:

1. Fork the [OpenWeave-Core
   repository](https://github.com/openweave/openweave-core) by clicking "Fork"
   on the web UI.

2. Enable [Travis CI](https://travis-ci.org/) by logging in with your GitHub
   account and enabling your newly created fork.  We use Travis CI for
   Linux-based continuous integration checks.  All contributions must pass these
   checks to be accepted.

Setup your local development environment:

```bash
# Clone your fork
git clone git@github.com:<username>/openweave-core.git

# Configure upstream alias
git remote add upstream git@github.com:openweave/openweave-core.git
```

### Before you contribute

Before we can use your code, you must sign the [Google Individual
Contributor License Agreement][CLA-INDI] (CLA), which you can do
online. The CLA is necessary mainly because you own the copyright to
your changes, even after your contribution becomes part of our
codebase, so we need your permission to use and distribute your code.
We also need to be sure of various other things—for instance that
you'll tell us if you know that your code infringes on other people's
patents. You don't have to sign the CLA until after you've submitted
your code for review and a member has approved it, but you must do it
before we can put your code into our codebase. Before you start
working on a larger contribution, you should get in touch with us
first through the issue tracker with your idea so that we can help out
and possibly guide you. Coordinating up front makes it much easier to
avoid frustration later on.

[CLA-INDI]: https://cla.developers.google.com/about/google-individual

### Submitting a Pull Request

#### Branch

For each new feature, create a working branch:

```bash
# Create a working branch for your new feature
git branch --track <branch-name> origin/master

# Checkout the branch
git checkout <branch-name>
```

#### Create Commits

```bash
# Add each modified file you'd like to include in the commit
git add <file1> <file2>

# Create a commit
git commit
```

This will open up a text editor where you can craft your commit message.

#### Upstream Sync and Clean Up

Prior to submitting your pull request, you might want to do a few things to
clean up your branch and make it as simple as possible for the original
repository's maintainer to test, accept, and merge your work.

If any commits have been made to the upstream master branch, you should rebase
your development branch so that merging it will be a simple fast-forward that
won't require any conflict resolution work.

```bash
# Fetch upstream master and merge with your repository's master branch
git checkout master
git pull upstream master

# If there were any new commits, rebase your development branch
git checkout <branch-name>
git rebase master
```

Now, it may be desirable to squash some of your smaller commits down into a
small number of larger more cohesive commits. You can do this with an
interactive rebase:

```bash
# Rebase all commits on your development branch
git checkout
git rebase -i master
```

This will open up a text editor where you can specify which commits to squash.

#### Coding Conventions and Style

OpenWeave Core runs a style check on every pull request as part of the
continuous integration and pull request validation.  In order to validate that
the submitted code meets the guidelines, you may run:

```bash
make -f Makefile-Standalone pretty-check
```

If you wish to use automated formatting tools, we have provided a `clang-format`
style.  In order to automatically format the file, you'd run:

```bash
clang-format -i -style=file <src-filename>
```

#### Push and Test

```bash
# Checkout your branch
git checkout <branch-name>

# Push to your GitHub fork:
git push origin <branch-name>
```

This will trigger the Travis CI continuous-integration checks.  You
can view the results in the respective services.  Note that the integration
checks will report failures on occasion.  If a failure occurs, you may try
rerunning the test via the Travis web UI.

#### Submit Pull Request

Once you've validated the Travis CI results, go to the page for
your fork on GitHub, select your development branch, and click the pull request
button. If you need to make any adjustments to your pull request, just push the
updates to GitHub. Your pull request will automatically track the changes on
your development branch and update.

#### Code reviews

All submissions, including submissions by project members, require review. 

### Documentation

Documentation undergoes the same review process as code and
contributions may be mirrored on our [openweave.io][ow-io] website.
See the [Documentation Style Guide][doc-style] for more information on
how to author and format documentation for contribution.

### The small print

Contributions made by corporations are covered by a different
agreement than the one above, the [Software Grant and Corporate
Contributor License Agreement][CLA-CORP].

[CLA-CORP]: https://cla.developers.google.com/about/google-corporate
[doc-style]: /doc/STYLE_GUIDE.md
[ow-io]: https://openweave.io
