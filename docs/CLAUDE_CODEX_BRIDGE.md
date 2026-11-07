# Claude ↔ Codex on GitHub (opt-in, without API billing)

This proposal connects the **existing Codex PR reviews** to Claude Code on
`Kerberone/Lichii`, using Anthropic's official
[`claude-code-action`](https://github.com/anthropics/claude-code-action).
It does not install or activate a paid OpenAI/Anthropic API integration.

## Owner-only setup after inspecting/merging this PR

1. Install/authorize the [official Claude GitHub App](https://github.com/apps/claude)
   for **Kerberone/Lichii** (GitHub repository owner action).
2. If you have an eligible Claude subscription, sign in to Claude Code locally
   under that subscription and run `claude setup-token` to obtain a Claude Code
   OAuth token. **Never put the token into a PR, comment, workflow file, message,
   or public log.**
3. Add it in **Settings → Secrets and variables → Actions → New repository secret**:
   name `CLAUDE_CODE_OAUTH_TOKEN`. Only the owner should set this secret.
   Do **not** add `ANTHROPIC_API_KEY` or `OPENAI_API_KEY`.
4. Merge this workflow *only when ready to test*. The workflow file alone is
   insufficient: both the GitHub App and a working secret are required.
5. Test with an **owner-authored comment** on an open Lichii PR whose head branch
   belongs to Kerberone and is **not `nx2`**. The command must **start** the comment:
   `@claude Please review the most recent Codex findings without making changes.`
   Incidental mentions elsewhere in a comment are deliberately ignored. Inspect
   the Actions run and Claude's PR response.
6. After confirming authentication, the second job can automatically perform
   a limited read-only follow-up on new Codex **review submissions** (not on
   every bot comment). No automated merge or push is allowed in that job.

## Workflow behavior and limits

- Only comments from the **Kerberone** GitHub user that **start** with `@claude `
  (or are exactly `@claude`) may manually invoke Claude; comments with the
  standard Claude Code footer are explicitly excluded to reduce self-triggering.
  A GitHub tool posting as Kerberone cannot otherwise be distinguished from a
  human posting as Kerberone. The PR must belong to this repository, be open,
  use a non-default feature branch and have been opened by Kerberone.
- The automatic job runs only when the **Codex connector bot** submits a review
  containing the Codex review receipt on an open PR owned by Kerberone **and**
  the review has an inline finding with the Codex finding marker. Clean reviews
  do not consume Claude usage. Each job checks out the explicitly identified
  PR commit; the automatic job is read-only.
- Its token has *read-only source* permissions, and its Claude tool list is
  restricted to reading/grepping. It is **not** an automated code-fixer.
  To request a correction, the owner can comment `@claude Fix the confirmed
  findings on this PR branch and explain what you tested`.
- Neither job invokes the other bot. Each invocation has a time/turn limit.
  Claude's response is published on GitHub, where ChatGPT can read it during
  a later check. GitHub comments do **not** directly wake this ChatGPT thread.
- No changes are pushed directly to `nx2`; human review and merge remain required.
- This workflow will **not** run until merged into `nx2`. GitHub evaluates
  some review event workflows against the PR base branch; PR #1 currently
  targets `audit/windows-pgo-validated`, so the automatic follow-up may not
  run there until the workflow exists in that base branch or future PRs target
  `nx2`. The manual command uses the workflow on the default branch.
- The action versions are pinned to their verified commit hashes rather
  than mutable tags.
- It won't automatically start an existing claude.ai web conversation: it
  launches a new Claude Code action run with the PR context.

## Cost and reliability caveats

- The intent is **zero additional API billing**, drawing on whatever Claude
  Code subscription usage is available. Subscription quotas still apply.
- Anthropic's documentation lists `claude setup-token` as supported, but users
  have reported `401` authentication failures for subscription OAuth tokens in
  GitHub Actions (see
  [anthropics/claude-code-action#1614](https://github.com/anthropics/claude-code-action/issues/1614)).
  This integration **must be tested** and cannot be guaranteed to work on your
  subscription. Do not switch to paid API credentials as a workaround without
  explicitly choosing to do so.
- Public repo Actions logs are public; do not enable verbose/full model output,
  paste credentials, or upload game assets or raw captures.
- The selected `@claude` phrase is user-triggered only. Bot-issued GitHub
  comments may not trigger other workflows when they originate from
  `GITHUB_TOKEN`, so the automatic job listens to Codex's own review event
  rather than asking GitHub Actions to post `@claude`.
- Review runs consume GitHub Actions minutes, which are free on standard runners
  for public repositories under GitHub's current policy.

## Optional rollback

Remove or disable the workflow, or revoke/delete the
`CLAUDE_CODE_OAUTH_TOKEN` GitHub Actions secret. Neither operation changes
emulator code or repository visibility.
