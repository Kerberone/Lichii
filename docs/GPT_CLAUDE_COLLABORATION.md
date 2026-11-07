# Optional GPT reviewer for Claude/Lichii collaboration

This is an **opt-in**, read-only code review workflow. It does not compile
emulator code, push changes, or merge pull requests.

## Enable it

1. Review and merge the setup pull request into `nx2`.
2. In GitHub, open **Settings → Secrets and variables → Actions**.
3. Add a *repository secret* named `OPENAI_API_KEY`, using a key created for
   your OpenAI API project. **Never paste the key into code or PR comments.**
4. Optionally add an Actions *variable* `LICHII_OPENAI_MODEL`. Without it,
   the workflow uses `gpt-6-astra`.
5. Have Claude work on a **branch + pull request** rather than pushing code
   straight to `nx2`. Each new push to that PR triggers a new GPT review.
   A PR must be non-draft and its branch must belong to this repository.
6. For a one-off follow-up, an owner/member/collaborator can comment
   `@gpt-review <question>` on a PR. You can also trigger the workflow
   manually from the Actions tab with a PR number.

The reviewer reads **only GitHub metadata and a bounded subset of the diff**.
Reviews are comments on the PR. They may miss changes outside that subset and
are not proof that the code builds or behaves correctly.

## Limitations

- ChatGPT Plus does **not** include API usage. OpenAI API calls are billed
  separately. Set spending limits in your API account.
- The reviewer is an OpenAI API model, **not the live ChatGPT conversation**.
- A push directly to `nx2`, with no open pull request, does not start a review.
- Claude Code does **not** automatically respond to a comment made by the
  standard GitHub Actions token. For fully automatic two-way dialogue, use a
  separately authenticated GitHub App/PAT with tightly scoped permissions
  and loop/cost limits; do not enable unbounded bot-to-bot replies.
- For safety, the workflow never checks out or executes the PR's changed code
  with an API key present.
- On fork-originated PRs, the workflow is disabled, to avoid exposing secrets
  or paying for arbitrary public contributions.

## Recommended workflow

Claude: implement changes on a branch, open/update a PR, include an explicit
technical question in the PR description.

GPT: automatically review the new diff, identify actionable correctness
issues and uncertainty, and leave an English review with an Italian summary.

Human: decide which suggestions to implement and approve merges. Test builds
on real hardware (particularly AMD Vulkan) before interpreting results.
