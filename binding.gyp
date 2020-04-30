{
  'targets': [
    {
      'target_name': 'auth_pam',
      'sources': [ 'src/main.c', 'src/auth-pam.c' ],
      'libraries': ['-lpam']
    }
  ]
}
