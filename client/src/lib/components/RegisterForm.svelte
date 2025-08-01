<script lang="ts">
	let formData = $state({
		firstName: '',
		lastName: '',
		email: '',
		password: '',
		confirmPassword: ''
	});
	let isLoading = $state(false);
	let errors = $state<{
		firstName?: string;
		lastName?: string;
		email?: string;
		password?: string;
		confirmPassword?: string;
		general?: string;
	}>({});

	async function handleRegister(event: Event) {
		event.preventDefault();
		isLoading = true;
		errors = {};

		// Validation
		if (!formData.firstName.trim()) {
			errors.firstName = 'First name is required';
		}

		if (!formData.lastName.trim()) {
			errors.lastName = 'Last name is required';
		}

		if (!formData.email) {
			errors.email = 'Email is required';
		} else if (!/\S+@\S+\.\S+/.test(formData.email)) {
			errors.email = 'Please enter a valid email';
		}

		if (!formData.password) {
			errors.password = 'Password is required';
		} else if (formData.password.length < 8) {
			errors.password = 'Password must be at least 8 characters';
		} else if (!/(?=.*[a-z])(?=.*[A-Z])(?=.*\d)/.test(formData.password)) {
			errors.password = 'Password must contain at least one uppercase letter, one lowercase letter, and one number';
		}

		if (!formData.confirmPassword) {
			errors.confirmPassword = 'Please confirm your password';
		} else if (formData.password !== formData.confirmPassword) {
			errors.confirmPassword = 'Passwords do not match';
		}

		if (Object.keys(errors).length > 0) {
			isLoading = false;
			return;
		}

		try {
			// Simulate API call
			await new Promise(resolve => setTimeout(resolve, 1500));
			console.log('Registration attempt:', formData);
			// Handle successful registration here
		} catch (error) {
			errors.general = 'Registration failed. Please try again.';
		} finally {
			isLoading = false;
		}
	}
</script>

<div class="min-h-screen flex items-center justify-center bg-gray-50 py-12 px-4 sm:px-6 lg:px-8">
	<div class="max-w-md w-full space-y-8">
		<div>
			<h2 class="mt-6 text-center text-3xl font-extrabold text-gray-900">
				Create your account
			</h2>
			<p class="mt-2 text-center text-sm text-gray-600">
				Already have an account?
				<a href="/login" class="font-medium text-indigo-600 hover:text-indigo-500">
					Sign in here
				</a>
			</p>
		</div>
		
		<form class="mt-8 space-y-6" onsubmit={handleRegister}>
			{#if errors.general}
				<div class="rounded-md bg-red-50 p-4">
					<div class="text-sm text-red-700">
						{errors.general}
					</div>
				</div>
			{/if}

			<div class="space-y-4">
				<div class="grid grid-cols-2 gap-4">
					<div>
						<label for="firstName" class="block text-sm font-medium text-gray-700">
							First name
						</label>
						<input
							id="firstName"
							name="firstName"
							type="text"
							autocomplete="given-name"
							bind:value={formData.firstName}
							class="mt-1 appearance-none relative block w-full px-3 py-2 border border-gray-300 placeholder-gray-500 text-gray-900 rounded-md focus:outline-none focus:ring-indigo-500 focus:border-indigo-500 sm:text-sm"
							placeholder="First name"
						/>
						{#if errors.firstName}
							<p class="mt-1 text-sm text-red-600">{errors.firstName}</p>
						{/if}
					</div>
					
					<div>
						<label for="lastName" class="block text-sm font-medium text-gray-700">
							Last name
						</label>
						<input
							id="lastName"
							name="lastName"
							type="text"
							autocomplete="family-name"
							bind:value={formData.lastName}
							class="mt-1 appearance-none relative block w-full px-3 py-2 border border-gray-300 placeholder-gray-500 text-gray-900 rounded-md focus:outline-none focus:ring-indigo-500 focus:border-indigo-500 sm:text-sm"
							placeholder="Last name"
						/>
						{#if errors.lastName}
							<p class="mt-1 text-sm text-red-600">{errors.lastName}</p>
						{/if}
					</div>
				</div>

				<div>
					<label for="email" class="block text-sm font-medium text-gray-700">
						Email address
					</label>
					<input
						id="email"
						name="email"
						type="email"
						autocomplete="email"
						bind:value={formData.email}
						class="mt-1 appearance-none relative block w-full px-3 py-2 border border-gray-300 placeholder-gray-500 text-gray-900 rounded-md focus:outline-none focus:ring-indigo-500 focus:border-indigo-500 sm:text-sm"
						placeholder="Enter your email"
					/>
					{#if errors.email}
						<p class="mt-1 text-sm text-red-600">{errors.email}</p>
					{/if}
				</div>
				
				<div>
					<label for="password" class="block text-sm font-medium text-gray-700">
						Password
					</label>
					<input
						id="password"
						name="password"
						type="password"
						autocomplete="new-password"
						bind:value={formData.password}
						class="mt-1 appearance-none relative block w-full px-3 py-2 border border-gray-300 placeholder-gray-500 text-gray-900 rounded-md focus:outline-none focus:ring-indigo-500 focus:border-indigo-500 sm:text-sm"
						placeholder="Create a password"
					/>
					{#if errors.password}
						<p class="mt-1 text-sm text-red-600">{errors.password}</p>
					{/if}
					<p class="mt-1 text-xs text-gray-500">
						Must be at least 8 characters with uppercase, lowercase, and number
					</p>
				</div>

				<div>
					<label for="confirmPassword" class="block text-sm font-medium text-gray-700">
						Confirm password
					</label>
					<input
						id="confirmPassword"
						name="confirmPassword"
						type="password"
						autocomplete="new-password"
						bind:value={formData.confirmPassword}
						class="mt-1 appearance-none relative block w-full px-3 py-2 border border-gray-300 placeholder-gray-500 text-gray-900 rounded-md focus:outline-none focus:ring-indigo-500 focus:border-indigo-500 sm:text-sm"
						placeholder="Confirm your password"
					/>
					{#if errors.confirmPassword}
						<p class="mt-1 text-sm text-red-600">{errors.confirmPassword}</p>
					{/if}
				</div>
			</div>

			<div class="flex items-center">
				<input
					id="terms"
					name="terms"
					type="checkbox"
					required
					class="h-4 w-4 text-indigo-600 focus:ring-indigo-500 border-gray-300 rounded"
				/>
				<label for="terms" class="ml-2 block text-sm text-gray-900">
					I agree to the
					<a href="/terms" class="text-indigo-600 hover:text-indigo-500">Terms of Service</a>
					and
					<a href="/privacy" class="text-indigo-600 hover:text-indigo-500">Privacy Policy</a>
				</label>
			</div>

			<div>
				<button
					type="submit"
					disabled={isLoading}
					class="group relative w-full flex justify-center py-2 px-4 border border-transparent text-sm font-medium rounded-md text-white bg-indigo-600 hover:bg-indigo-700 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-indigo-500 disabled:opacity-50 disabled:cursor-not-allowed transition-colors duration-200"
				>
					{#if isLoading}
						<svg class="animate-spin -ml-1 mr-3 h-5 w-5 text-white" xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24">
							<circle class="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" stroke-width="4"></circle>
							<path class="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z"></path>
						</svg>
						Creating account...
					{:else}
						Create account
					{/if}
				</button>
			</div>
		</form>
	</div>
</div>
