# Complex mathematics

This document exercises the current native math profile. Each numbered section
is a separate visual review case. It uses no macros, packages or external fonts.
Open the [sample guide](README.md) for the edit, undo and presentation checks.

## M01 — Inline baselines and simultaneous scripts

An indexed iterate $x_i^{k+1}$, a fraction $\frac{a+b}{1+c^2}$ and an indexed
root $\sqrt[3]{1+t^2}$ should fit the surrounding sentence. Compare the slash
envelope \( \left\langle u,v\right\rangle \) and the identity
$\sin^2\theta+\cos^2\theta=1$ without changing their exact source.

## M02 — Nested fractions and an indexed root

$$F(x)=\frac{1+\frac{x^2}{1+x}}{\sqrt[3]{1+\frac{1}{1+x^2}}}$$

Check the three fraction bars and the radical's coverage independently.

## M03 — A finite continued fraction

$$C=1+\frac{1}{2+\frac{1}{3+\frac{1}{4+\frac{1}{5}}}}$$

The nested denominators must remain distinct at small text sizes.

## M04 — Display operator limits

$$S_n=\sum\limits_{k=1}^{n}\frac{k^2}{1+k^4},\qquad P_n=\prod\limits_{k=1}^{n}\left(1+\frac{1}{k^2}\right)$$

## M05 — Side limits and explicit atom classes

$$\sum\nolimits_{i=1}^{n}a_i\mathbin{*}b_i\mathrel{=}\mathop{score}\limits_{x}\left(x^2+1\right)$$

This is a spacing/layout specimen, not a mathematical identity being asserted.

## M06 — Integral with an evaluation bracket

$$\int\limits_0^1 x^3\,\mathrm{d}x=\left.\frac{x^4}{4}\right|_0^1=\frac{1}{4}$$

## M07 — Gaussian density and normalization

$$p(x)=\frac{1}{\sigma\sqrt{2\pi}}\exp\left(-\frac{(x-\mu)^2}{2\sigma^2}\right),\qquad\int_{-\infty}^{\infty}p(x)\,\mathrm{d}x=1$$

Assume a positive scale parameter in this mathematical example.

## M08 — Complex exponentials

$$\mathrm{e}^{\mathrm{i}\theta}=\cos\theta+\mathrm{i}\sin\theta,\qquad z\overline{z}=|z|^2$$

## M09 — Accents and styled alphabets

$$\widehat{ABC}+\widetilde{XYZ}+\overline{a+b}+\underline{x_i}+\vec{v}+\dot{x}+\ddot{x}$$

$$\mathbb{R}^{n}\supset\mathcal{V},\qquad\mathbf{A}\mathbf{x}=\mathbf{b},\qquad\mathit{velocity}=\mathrm{distance}/\mathrm{time}$$

## M10 — Nested scalable delimiters

$$\left\lVert\left(\frac{a+b}{c+d},\sqrt{1+t^2}\right)\right\rVert^2+\left\lfloor\frac{n+1}{2}\right\rfloor$$

## M11 — Explicit delimiter sizes

$$\bigl(x\bigr)+\Bigl(\frac{x}{2}\Bigr)+\biggl(\frac{1}{1+x^2}\biggr)+\Biggl(\frac{1}{\sqrt{1+x^2}}\Biggr)$$

Inspect all four requested sizes; font-dependent geometry need not equal TeX.

## M12 — A matrix containing fractions, roots and scripts

$$A=\begin{pmatrix}1&\frac{1}{2}&\sqrt{2}\\x_1^2&1+\alpha&\frac{a+b}{c}\\0&\overline{z}&1\end{pmatrix}$$

## M13 — Matrix multiplication and a column vector

$$\begin{bmatrix}2&-1&0\\-1&2&-1\\0&-1&2\end{bmatrix}\begin{pmatrix}x_1\\x_2\\x_3\end{pmatrix}=\begin{pmatrix}b_1\\b_2\\b_3\end{pmatrix}$$

## M14 — A four-by-four determinant

$$D=\begin{vmatrix}1&a&a^2&a^3\\1&b&b^2&b^3\\1&c&c^2&c^3\\1&d&d^2&d^3\end{vmatrix}$$

## M15 — A denser six-by-six matrix

$$T=\begin{bmatrix}2&-1&0&0&0&0\\-1&2&-1&0&0&0\\0&-1&2&-1&0&0\\0&0&-1&2&-1&0\\0&0&0&-1&2&-1\\0&0&0&0&-1&2\end{bmatrix}$$

There are six rows and six columns. Check every cell, especially the two corners
and the final row after scrolling.

## M16 — Piecewise branches with nested expressions

$$f(x)=\begin{cases}\frac{x^2}{1+x^2}&x<0\\\sqrt{1+x}-1&0\leq x<1\\\frac{1}{2}+\log x&x\geq1\end{cases}$$

## M17 — Aligned derivation

$$\begin{aligned}(a+b)^2&=a^2+2ab+b^2\\(a-b)^2&=a^2-2ab+b^2\\(a+b)^2-(a-b)^2&=4ab\end{aligned}$$

The relation columns should line up despite unequal left-hand widths.

## M18 — An aligned system with two alignment pairs

$$\begin{aligned}x+y&=3,&\quad x-y&=1\\2x+y&=5,&\quad x+2y&=4\end{aligned}$$

## M19 — Gathered vector identities

$$\begin{gathered}\mathbf{u}=\begin{pmatrix}u_1\\u_2\end{pmatrix},\quad\mathbf{v}=\begin{pmatrix}v_1\\v_2\end{pmatrix}\\\left\langle\mathbf{u},\mathbf{v}\right\rangle=u_1v_1+u_2v_2\end{gathered}$$

## M20 — Named optimization operator and norm

$$\widehat{\mathbf{x}}=\operatorname*{argmin}\limits_{\mathbf{x}\in\mathbb{R}^{n}}\left(\left\lVert A\mathbf{x}-b\right\rVert^2+\lambda\left\lVert\mathbf{x}\right\rVert^2\right)$$

## M21 — Mixed display and script styles

$$\displaystyle\sum_{i=1}^{n}\frac{x_i}{1+x_i^2}+{\textstyle\frac{a}{b}}+{\scriptstyle\frac{p}{q}}+{\scriptscriptstyle x^{y^z}}$$

## M22 — Slash display envelope and upright text

\[
\begin{aligned}
\operatorname{mean}(x)&=\frac{1}{n}\sum_{i=1}^{n}x_i\\
\operatorname{variance}(x)&=\frac{1}{n}\sum_{i=1}^{n}\left(x_i-\operatorname{mean}(x)\right)^2
\end{aligned}
\]

The entire display remains one logical Markdown paragraph.

## M23 — Text spacing and escaped braces

$$\text{speed in {m/s}}=\frac{\mathrm{distance}}{\mathrm{time}},\qquad\text{set \{a,b\}}$$

## M24 — Math inside ordinary Markdown containers

> A quoted bound is $\left|\frac{a}{b}\right|\leq\frac{|a|}{|b|}$ for nonzero $b$.

- The first list item contains $\sqrt{\frac{1+x^2}{1+y^2}}$.
- The next item contains $\alpha_i^{n+1}+\beta_j^{m-1}$ and ordinary **bold text**.

## M25 — Maxwell's equations in vacuum

$$\begin{aligned}\nabla\cdot\mathbf{E}&=\frac{\rho}{\varepsilon_0}\\\nabla\cdot\mathbf{B}&=0\\\nabla\times\mathbf{E}&=-\frac{\partial\mathbf{B}}{\partial t}\\\nabla\times\mathbf{B}&=\mu_0\mathbf{J}+\mu_0\varepsilon_0\frac{\partial\mathbf{E}}{\partial t}\end{aligned}$$

Check the different relation widths, vector styling, fractions and Greek scripts
without changing the alignment of the four equations.

## M26 — A time-dependent wave equation

$$\mathrm{i}\hbar\frac{\partial\psi}{\partial t}=-\frac{\hbar^2}{2m}\nabla^2\psi+V\psi$$

## M27 — A Hessian with mixed partial derivatives

$$H_f=\begin{bmatrix}\frac{\partial^2 f}{\partial x^2}&\frac{\partial^2 f}{\partial x\partial y}\\\frac{\partial^2 f}{\partial y\partial x}&\frac{\partial^2 f}{\partial y^2}\end{bmatrix}$$

Finish by editing one matrix entry with F2, applying it, undoing and redoing it.
Then cancel a different draft. The surrounding Markdown must stay unchanged.
