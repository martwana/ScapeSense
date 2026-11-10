import React, { useEffect, useRef } from "react";
import {
	FISH_EYE,
	FISH_PATHS,
	bubblePalette,
	createFish,
	fishPose,
	layoutPlants,
	leafShape,
	leafSway,
	substrateBands,
} from "../lib/scene.js";

const PLANT_COLOUR = "#103030";
const SUBSTRATE_COLOUR = "#0d2729";
const FISH_COLOUR = "#143c3e";

function fillBand(ctx, points, bottom) {
	ctx.beginPath();
	ctx.moveTo(points[0].x, points[0].y);
	for (const point of points) ctx.lineTo(point.x, point.y);
	if (typeof bottom === "number") {
		ctx.lineTo(points.at(-1).x, bottom);
		ctx.lineTo(points[0].x, bottom);
	} else {
		for (let i = points.length - 1; i >= 0; i -= 1) {
			ctx.lineTo(points[i].x, points[i].y + bottom.thickness);
		}
	}
	ctx.closePath();
	ctx.fill();
}

// Leaf silhouette from the logo: a curved lance shape with an off-centre
// vein cut out of the blade.
function drawLeaf(ctx, leaf, t) {
	const sway = leafSway(leaf, t);
	const { mirrored, stem, bladeRotation, blade, vein } = leafShape(leaf);
	ctx.save();
	ctx.translate(leaf.x, leaf.y);
	ctx.rotate(leaf.angle + sway);
	if (mirrored) ctx.scale(-1, 1);

	ctx.lineWidth = stem.lineWidth;
	ctx.beginPath();
	ctx.moveTo(0, 0);
	ctx.quadraticCurveTo(stem.control.x, stem.control.y, stem.tip.x, stem.tip.y);
	ctx.stroke();

	ctx.translate(stem.tip.x, stem.tip.y);
	ctx.rotate(bladeRotation + sway * 0.6);
	ctx.beginPath();
	ctx.moveTo(0, 0);
	for (const curve of blade) ctx.bezierCurveTo(...curve);
	ctx.fill();

	ctx.globalCompositeOperation = "destination-out";
	ctx.lineWidth = vein.lineWidth;
	ctx.beginPath();
	ctx.moveTo(vein.from.x, vein.from.y);
	ctx.quadraticCurveTo(vein.control.x, vein.control.y, vein.to.x, vein.to.y);
	ctx.stroke();
	ctx.restore();
}

function drawFish(ctx, paths, pose) {
	ctx.save();
	ctx.translate(pose.x, pose.y);
	ctx.rotate(pose.tilt * pose.direction);
	ctx.scale(pose.direction * pose.scale, pose.scale);
	for (const path of paths) ctx.fill(path);
	ctx.globalCompositeOperation = "destination-out";
	ctx.beginPath();
	ctx.arc(FISH_EYE.x, FISH_EYE.y, FISH_EYE.r, 0, Math.PI * 2);
	ctx.fill();
	ctx.restore();
}

// Ambient tank inspired by the ScapeSense logo: a planted bottom-left corner
// feeding a fine CO₂ mist, a wavy substrate and a few fish drifting past.
export default function AquariumBackground({ theme = 'dark' }) {
	const canvasRef = useRef(null);
	const paletteRef = useRef(bubblePalette(theme));
	useEffect(() => { paletteRef.current = bubblePalette(theme); }, [theme]);

	useEffect(() => {
		const canvas = canvasRef.current;
		const ctx = canvas.getContext("2d");
		const reducedMotion = window.matchMedia(
			"(prefers-reduced-motion: reduce)",
		).matches;
		const fishPaths = FISH_PATHS.map((path) => new Path2D(path));
		const fish = createFish();

		let width = 0;
		let height = 0;
		let bubbles = [];
		let plants = [];
		let substrate = null;
		let raf = 0;

		const rand = (min, max) => min + Math.random() * (max - min);

		const spawn = (initial = false) => {
			const x = rand(6, Math.min(48, width * 0.08));
			const big = Math.random() < 0.05;
			return {
				x,
				originX: x,
				spread: rand(0.03, 0.98),
				delay: initial ? rand(0, 18) : 0,
				y: height + rand(4, 24),
				r: big ? rand(2.6, 4.4) : rand(0.6, 2.2),
				rise: big ? rand(40, 70) : rand(14, 40),
				wobbleAmp: rand(4, 16),
				wobbleFreq: rand(0.7, 2.1),
				phase: rand(0, Math.PI * 2),
				alpha: rand(0.1, 0.38),
				tintIndex: Math.floor(Math.random() * 3),
			};
		};

		const drawScenery = (t) => {
			ctx.lineCap = "round";
			ctx.fillStyle = SUBSTRATE_COLOUR;
			fillBand(ctx, substrate.hill, height);
			fillBand(ctx, substrate.ribbon, substrate);

			ctx.fillStyle = PLANT_COLOUR;
			ctx.strokeStyle = PLANT_COLOUR;
			for (const leaf of plants) drawLeaf(ctx, leaf, t);

			ctx.fillStyle = FISH_COLOUR;
			for (const one of fish) {
				drawFish(ctx, fishPaths, fishPose(one, t, width, height));
			}
		};

		const drawBubbles = (t, dt) => {
			const palette = paletteRef.current;
			for (let i = 0; i < bubbles.length; i += 1) {
				const b = bubbles[i];

				if (b.delay > 0) {
					b.delay -= dt;
					continue;
				}
				const progress = Math.max(0, Math.min(1, (height - b.y) / height));
				const plumeLift = Math.exp(-progress * 8);
				const nearest = b.originX + (width - b.originX - 8) * b.spread * progress;

				// Converging plumes: nearby water is dragged toward the stream
				// and accelerated upward, so bubbles gather into columns.
				const pull = Math.max(-26, Math.min(26, (nearest - b.x) * 0.22));
				const current =
					Math.sin(b.y * 0.004 + t * 0.32) * 13 +
					Math.sin(t * b.wobbleFreq + b.phase + b.y * 0.012) *
						b.wobbleAmp;
				const vy =
					b.rise * (0.75 + 1.05 * plumeLift + 0.22 * Math.sin(t * 0.3 + b.phase));

				const outward = (width - b.originX - 8) * b.spread * vy / height;
				b.x += (outward + current * (0.2 + progress) + pull) * dt;
				b.y -= vy * dt;
				b.x = Math.max(b.r + 2, Math.min(width - b.r - 2, b.x));

				if (b.y < -12) {
					bubbles[i] = spawn(false);
					continue;
				}

				const fade = Math.min(1, b.y / 80, (height - b.y) / 50);
				ctx.globalAlpha = Math.max(0, b.alpha * fade * palette.opacity);
				ctx.fillStyle = palette.tints[b.tintIndex];
				ctx.beginPath();
				ctx.arc(b.x, b.y, b.r, 0, Math.PI * 2);
				ctx.fill();

				if (b.r > 2.4) {
					ctx.globalAlpha = Math.max(0, b.alpha * fade * 0.9 * palette.opacity);
					ctx.fillStyle = palette.highlight;
					ctx.beginPath();
					ctx.arc(b.x - b.r * 0.3, b.y - b.r * 0.3, b.r * 0.28, 0, Math.PI * 2);
					ctx.fill();
				}
			}
			ctx.globalAlpha = 1;
		};

		const resize = () => {
			const dpr = Math.min(window.devicePixelRatio || 1, 2);
			width = window.innerWidth;
			height = window.innerHeight;
			canvas.width = Math.round(width * dpr);
			canvas.height = Math.round(height * dpr);
			canvas.style.width = `${width}px`;
			canvas.style.height = `${height}px`;
			ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
			plants = layoutPlants(width, height);
			substrate = substrateBands(width, height);
			const target = Math.min(
				170,
				Math.max(50, Math.round((width * height) / 15000)),
			);
			bubbles = Array.from({ length: target }, () => spawn(true));
			if (reducedMotion) {
				ctx.clearRect(0, 0, width, height);
				drawScenery(0);
			}
		};

		let last = performance.now();
		const step = (now) => {
			const dt = Math.min(0.05, (now - last) / 1000);
			last = now;
			const t = now / 1000;
			ctx.clearRect(0, 0, width, height);
			drawScenery(t);
			drawBubbles(t, dt);
			raf = requestAnimationFrame(step);
		};

		resize();
		window.addEventListener("resize", resize);
		if (!reducedMotion) raf = requestAnimationFrame(step);

		return () => {
			cancelAnimationFrame(raf);
			window.removeEventListener("resize", resize);
		};
	}, []);

	return (
		<canvas
			ref={canvasRef}
			aria-hidden='true'
			className='pointer-events-none fixed inset-0 z-0'
		/>
	);
}
